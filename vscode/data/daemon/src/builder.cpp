#include "stk500v2.h"
#include "builder.h"
#include "daemon_state.h"
#include "utils.h"
#include "port_scanner.h"
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <future>
#include <set>
#include <sstream>
#include <thread>
namespace fs = std::filesystem;

// =============================================================================

// 全体の処理フロー (要約)
//
//   compile(req)
//     ├ resolveSketch()     : .ino ファイルパス / ディレクトリパスを正規化
//     ├ resolveWorkspace()  : .vscode/ を持つフォルダを上方向に探索
//     ├ ensureCoreArchive() : core.a が無ければ arduino-cli で 1 回生成
//     ├ collectSources()    : 全 .ino/.cpp/.c とそのスタンプを列挙
//     ├ loadStamps()        : 前回の (mtime,size) を .stamps から読む
//     ├ planRebuild()       : 変更ファイルを特定 (差分判定の核心)
//     ├ runCompiles()       : 変更ファイルだけ avr-g++/avr-gcc を呼ぶ
//     ├ runLink()           : 全 .o + core.a を avr-gcc でリンク
//     ├ runObjcopy()        : .elf → .hex
//     └ saveStamps()        : .stamps を更新
//
// 設計原則:
//   - すべてのデータは「型」として明示する (ぼやっとした文字列は使わない)
//   - 各フェーズは 1 つの関数として独立し、副作用を局所化する
//   - キャッシュ判定は (size, mtime) のみ。ハッシュは取らない (高速優先)

// =============================================================================

namespace {

	// =============================================================================

	// データ型

	// =============================================================================

	struct BoardConfig {
		std::string mcu;          // "atmega2560"
		std::string fcpu;         // "16000000L"
		std::string variant;      // "mega"
		std::string programmer;   // "wiring"
		long uploadBaud = 115200;
	};
	struct FileEntry {
		std::string srcPath;      // ソースのフルパス (.ino 統合の場合は build/<name>.ino.cpp)
		std::string objPath;      // 出力 .o のフルパス
		bool isCpp = true;        // false なら C コンパイラ
		long long mtime = 0;
		uintmax_t size = 0;
	};

	// =============================================================================

	// パス/環境ユーティリティ

	// =============================================================================

	// ポータブル版VSCodeのキャッシュディレクトリを特定
	const std::string& getPortableCacheDir() {
		// 実行ファイル位置はプロセス中に変化しないため、初回だけ解決する。
		static const std::string cacheDir = [] {
			char buf[MAX_PATH];
			GetModuleFileNameA(nullptr, buf, MAX_PATH);
			fs::path p = buf;

			auto toLower = [](std::string s) {
				std::transform(s.begin(), s.end(), s.begin(),
					[](unsigned char c) { return (char)std::tolower(c); });
				return s;
			};

			while (!p.empty() && p.parent_path() != p) {
				std::string name = toLower(p.filename().string());
				if (name.find("vscode-win32") != std::string::npos || name == "vscode") {
					return (p / "data" / "cache").string();
				}
				p = p.parent_path();
			}
			return (fs::path(buf).parent_path() / "data" / "cache").string();
			}();
		return cacheDir;
	}

	// .ino / フォルダ / 末尾スラッシュ を吸収して「.ino を含むディレクトリ」を返す
	std::string resolveSketchDir(const std::string& raw) {
		std::error_code ec;
		fs::path p(raw);
		if (p.extension() == ".ino") {
			if (fs::is_regular_file(p, ec)) return p.parent_path().string();
			return "";
		}
		if (fs::is_directory(p, ec)) {
			return p.string();   // .ino 有無は後続の1回のソース走査で確認
		}
		return "";
	}
	// hint があればそれ、なければ sketch から親方向に .vscode/ を探す
	std::string resolveWorkspace(const std::string& sketchDir, const std::string& hint) {
		std::error_code ec;
		if (!hint.empty()) {
			fs::path h = fs::absolute(hint, ec);
			if (!ec && fs::is_directory(h, ec)) return h.string();
		}
		fs::path p = fs::absolute(sketchDir, ec);
		if (ec) return sketchDir;
		while (!p.empty()) {
			std::error_code e2;
			if (fs::is_directory(p / ".vscode", e2)) return p.string();
			fs::path parent = p.parent_path();
			if (parent == p) break;
			p = parent;
		}
		return sketchDir;
	}

	// ポータブルディレクトリの cache/build/<スケッチ名>_<ハッシュ> に保存
	std::string computeBuildDir(const std::string& workspace, const std::string& sketch) {
		std::error_code ec;
		fs::path cacheRoot = getPortableCacheDir();
		fs::path sd = fs::absolute(sketch, ec);

		std::string leaf = sd.filename().string();
		if (leaf.empty()) leaf = "sketch";

		// 同名のスケッチ名が別ディレクトリにある場合の衝突回避
		std::string hash = Utils::sha1Hex(sd.string()).substr(0, 8);
		fs::path base = cacheRoot / "build" / (leaf + "_" + hash);

		return base.string();
	}

	// =============================================================================

	// ボード設定

	// =============================================================================

	BoardConfig resolveBoardConfig(const std::string& fqbn) {
		BoardConfig c;
		c.mcu = "atmega2560";
		c.fcpu = "16000000L";
		c.variant = "mega";
		c.programmer = "wiring";
		c.uploadBaud = 115200;
		std::string lower = fqbn;
		std::transform(lower.begin(), lower.end(), lower.begin(),
			[](unsigned char ch) { return (char)std::tolower(ch); });
		if (lower.find("cpu=atmega1280") != std::string::npos) {
			c.mcu = "atmega1280";
			c.programmer = "arduino";
			c.uploadBaud = 57600;
		}
		return c;
	}

	// =============================================================================

	// コンパイラフラグ生成

	// =============================================================================

	std::string commonDefines(const BoardConfig& bc) {
		std::ostringstream f;
		f << " -DF_CPU=" << bc.fcpu
			<< " -DARDUINO=10819"
			<< " -DARDUINO_AVR_MEGA2560"
			<< " -DARDUINO_ARCH_AVR";
		return f.str();
	}
	std::string includeFlags(const std::string& sketchDir, const std::string& buildDir) {
		std::ostringstream f;
		f << " -I\"" << g_state.toolchain.coreDir << "\"";
		if (!g_state.toolchain.variantDir.empty()) {
			f << " -I\"" << g_state.toolchain.variantDir << "\"";
		}
		f << " -I\"" << sketchDir << "\""
			<< " -I\"" << buildDir << "\"";
		return f.str();
	}
	std::string cppFlags(const BoardConfig& bc, const std::string& sketchDir,
		const std::string& buildDir) {
		std::ostringstream f;
		f << "-c -Os -pipe -Wall -Wextra -std=gnu++17"
			<< " -ffunction-sections -fdata-sections -fno-exceptions"
			<< " -fno-threadsafe-statics -Wno-error=narrowing"
			<< " -Wno-unused-variable"
			<< " -mmcu=" << bc.mcu
			<< commonDefines(bc)
			<< includeFlags(sketchDir, buildDir);
		return f.str();
	}
	std::string cFlags(const BoardConfig& bc, const std::string& sketchDir,
		const std::string& buildDir) {
		std::ostringstream f;
		f << "-c -Os -pipe -Wall -Wextra -std=gnu11"
			<< " -ffunction-sections -fdata-sections"
			<< " -Wno-unused-variable"
			<< " -mmcu=" << bc.mcu
			<< commonDefines(bc)
			<< includeFlags(sketchDir, buildDir);
		return f.str();
	}

	// =============================================================================

	// core.a キャッシュ

	// =============================================================================

	std::string coreCacheKey(const BoardConfig& bc) {
		std::ostringstream oss;
		oss << bc.mcu << "|" << bc.fcpu << "|" << bc.variant << "|"
			<< g_state.toolchain.compilerVersion << "|"
			<< Utils::getFileLastWriteTime(g_state.toolchain.coreDir);
		return Utils::sha1Hex(oss.str()).substr(0, 16);
	}

	// ポータブルディレクトリ内に cores フォルダを構築
	std::string coreArchivePath(const std::string& key) {
		fs::path cacheRoot = getPortableCacheDir();
		std::string dir = Utils::joinPath((cacheRoot / "cores").string(), key);
		Utils::createDirectory(dir);
		return Utils::joinPath(dir, "core.a");
	}

	std::string findArduinoCli() {
		char buf[MAX_PATH * 2];
		DWORD r = SearchPathA(nullptr, "arduino-cli.exe", nullptr, sizeof(buf), buf, nullptr);
		if (r > 0 && r < sizeof(buf)) return std::string(buf, r);
		std::vector<std::string> candidates = {
			Utils::joinPath(Utils::getLocalAppDataPath(), "Programs\\arduino-cli\\arduino-cli.exe"),
			"C:\\Program Files\\Arduino CLI\\arduino-cli.exe",
		};
		for (const auto& c : candidates) {
			if (Utils::fileExists(c)) return c;
		}
		return "";
	}

	// 初回のみ arduino-cli compile を呼んで core.a を取り出す
	bool ensureCoreArchive(const BoardConfig& bc, const std::string& sketchDir,
		const std::string& targetCoreA, std::string& errOut) {
		if (Utils::fileExists(targetCoreA)) return true;
		std::string cli = findArduinoCli();
		if (cli.empty()) {
			errOut = "arduino-cli not found (cannot generate core.a)";
			return false;
		}

		// テンポラリビルドもポータブル内へ作成
		std::string tmpBuild = Utils::joinPath(getPortableCacheDir(), "tmp-build");
		Utils::createDirectory(tmpBuild);

		std::string fqbn = (bc.variant == "megaADK")
			? "arduino:avr:megaADK"
			: "arduino:avr:mega:cpu=" + bc.mcu;
		std::ostringstream args;
		args << "compile --fqbn " << fqbn
			<< " --build-path \"" << tmpBuild << "\""
			<< " \"" << sketchDir << "\"";
		auto pr = runProcess(cli, args.str(), "", true);
		if (pr.exitCode != 0) {
			errOut = "arduino-cli compile failed:\n" + pr.error + "\n" + pr.output;
			return false;
		}
		std::string srcCoreA = Utils::joinPath(tmpBuild, "core\\core.a");
		if (!Utils::fileExists(srcCoreA)) {
			errOut = "core.a not produced by arduino-cli at " + srcCoreA;
			return false;
		}
		std::error_code ec;
		fs::copy_file(srcCoreA, targetCoreA, fs::copy_options::overwrite_existing, ec);
		if (ec) {
			errOut = "Failed to copy core.a: " + ec.message();
			return false;
		}
		return true;
	}

	// =============================================================================

	// .ino → .cpp 変換

	// =============================================================================

	// 入力 .ino 群の最終更新時刻 / 合計サイズを 1 度の走査で取得
	struct InoStats {
		long long maxMtime = 0;
		uint64_t totalSize = 0;
	};
	InoStats statInos(const std::vector<std::string>& inos) {
		InoStats s;
		for (const auto& p : inos) {
			long long t = 0; uint64_t sz = 0;
			if (Utils::getFileMetadata(p, t, sz)) {
				if (t > s.maxMtime) s.maxMtime = t;
				s.totalSize += sz;
			}
		}
		return s;
	}

	// 全 .ino を 1 つの .cpp に結合してファイルに書き出す
	//   内容が前回と同じなら mtime を変えないので、差分判定が誤動作しない
	//
	// 高速パス: 既存の .ino.cpp が「すべての .ino より新しい」なら、
	//   ファイルを読まずに即返す。Arduino スケッチは編集してから build を
	//   何度も叩く運用 (例: VS Code の onSave で 1 回ビルド) でも、ファイルを
	//   触っていない 2 回目以降は read+merge を完全にスキップできる。
	std::string mergeInoFiles(const std::vector<std::string>& inos,
		const std::string& sketchDir,
		const std::string& buildDir,
		long long inoMaxMtime) {
		std::string sketchName = Utils::getFileName(sketchDir);
		std::string outPath = Utils::joinPath(buildDir, sketchName + ".ino.cpp");

		long long outMtime = Utils::getFileLastWriteTime(outPath);
		if (outMtime != 0 && outMtime >= inoMaxMtime) {
			// .ino.cpp が全 .ino より新しい → 中身は前回と同じはず
			return outPath;
		}

		// 必要な容量を予測して 1 回で reserve
		size_t estSize = 64;
		for (const auto& ino : inos) {
			long long t = 0; uint64_t sz = 0;
			Utils::getFileMetadata(ino, t, sz);
			estSize += (size_t)sz + 64;
		}
		std::string out;
		out.reserve(estSize);
		out += "// Auto-generated by arduino-build-daemon\n";
		out += "#include <Arduino.h>\n";
		for (const auto& ino : inos) {
			out += "#line 1 \"";
			out += Utils::getFileName(ino);
			out += "\"\n";
			out += Utils::readFile(ino);
			out += '\n';
		}
		Utils::writeFileIfChanged(outPath, out);
		return outPath;
	}

	// =============================================================================

	// ソース列挙とプラン作成

	// =============================================================================

	// すべてのコンパイル単位を 1 度のディレクトリスキャンで列挙する。
	//   旧実装は .ino, .cpp, .c それぞれ別ループで 3 回スキャンしていた。
	//   ファイルメタデータも 1 回の GetFileAttributesEx で取る。
	std::vector<FileEntry> collectSources(const std::string& sketchDir,
		const std::string& buildDir, bool* hasIno = nullptr) {
		std::vector<std::string> inos;
		std::vector<std::string> cpps;
		std::vector<std::string> cs;

		// 1 パスで全 .ino/.cpp/.c を分類
		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(sketchDir, ec)) {
			if (ec) break;
			std::error_code rfEc;
			if (!entry.is_regular_file(rfEc)) continue;
			std::string ext = entry.path().extension().string();
			if (ext == ".ino")      inos.push_back(entry.path().string());
			else if (ext == ".cpp") cpps.push_back(entry.path().string());
			else if (ext == ".c")   cs.push_back(entry.path().string());
		}
		std::sort(inos.begin(), inos.end());
		std::sort(cpps.begin(), cpps.end());
		std::sort(cs.begin(), cs.end());
		if (hasIno) *hasIno = !inos.empty();

		std::vector<FileEntry> entries;
		entries.reserve(inos.empty() ? cpps.size() + cs.size()
			: 1 + cpps.size() + cs.size());

		// 1) .ino 群を 1 つの .ino.cpp に統合
		if (!inos.empty()) {
			InoStats st = statInos(inos);
			std::string merged = mergeInoFiles(inos, sketchDir, buildDir, st.maxMtime);
			FileEntry e;
			e.srcPath = merged;
			e.objPath = merged + ".o";
			e.isCpp = true;
			e.mtime = st.maxMtime;
			e.size = st.totalSize;
			entries.push_back(std::move(e));
		}
		// 2) .cpp
		for (const auto& src : cpps) {
			FileEntry e;
			e.srcPath = src;
			e.objPath = Utils::joinPath(buildDir, Utils::getFileStem(src) + ".cpp.o");
			e.isCpp = true;
			uint64_t sz = 0;
			Utils::getFileMetadata(src, e.mtime, sz);
			e.size = static_cast<uintmax_t>(sz);
			entries.push_back(std::move(e));
		}
		// 3) .c
		for (const auto& src : cs) {
			FileEntry e;
			e.srcPath = src;
			e.objPath = Utils::joinPath(buildDir, Utils::getFileStem(src) + ".c.o");
			e.isCpp = false;
			uint64_t sz = 0;
			Utils::getFileMetadata(src, e.mtime, sz);
			e.size = static_cast<uintmax_t>(sz);
			entries.push_back(std::move(e));
		}
		return entries;
	}

	// =============================================================================

	// .stamps ファイル: ファイル単位の (mtime,size) 永続化
	// フォーマット: 各行 "<mtime> <size> <srcPath>"

	// =============================================================================

	struct Stamp { long long mtime; uintmax_t size; };
	using StampMap = std::unordered_map<std::string, Stamp>;
	StampMap loadStamps(const std::string& buildDir) {
		StampMap m;
		std::string path = Utils::joinPath(buildDir, ".stamps");
		if (!Utils::fileExists(path)) return m;
		for (const auto& line : Utils::readLines(path)) {
			if (line.empty()) continue;
			std::istringstream iss(line);
			Stamp s{};
			std::string srcPath;
			if (iss >> s.mtime >> s.size) {
				std::getline(iss >> std::ws, srcPath);
				if (!srcPath.empty() && srcPath.back() == '\r') {
					srcPath.pop_back();
				}
				if (!srcPath.empty()) m[srcPath] = s;
			}
		}
		return m;
	}
	void saveStamps(const std::string& buildDir, const std::vector<FileEntry>& entries) {
		std::ostringstream oss;
		for (const auto& e : entries) {
			oss << e.mtime << " " << e.size << " " << e.srcPath << "\n";
		}
		Utils::writeFile(Utils::joinPath(buildDir, ".stamps"), oss.str());
	}

	// =============================================================================

	// 差分判定: どのファイルを再コンパイルすべきか

	// =============================================================================

	// gcc -MMD が出力する Make 形式の依存ファイルを読み、依存ファイルパスを返す。
	// パース方針:
	//   - 行末の "\<改行>" は行継続として1つの空白に潰す
	//   - ターゲット (1 個目のトークン) を区切る ":" 以降が依存リスト本体
	//   - "\ " はパス内のリテラルスペースとして扱う
	//   - Windows のドライブレター ("C:") を target の ":" と誤判定しないため、
	//     ":" は「直前がパス区切りまたは行頭で、その後ろが空白」の場合のみ
	//     セパレータとみなす
	std::vector<std::string> readDepFile(const std::string& depPath) {
		std::vector<std::string> deps;
		if (!Utils::fileExists(depPath)) return deps;
		std::string content = Utils::readFile(depPath);
		if (content.empty()) return deps;
		// 行継続と改行を空白に正規化
		std::string flat;
		flat.reserve(content.size());
		for (size_t i = 0; i < content.size(); ++i) {
			char c = content[i];
			if (c == '\\' && i + 1 < content.size()
				&& (content[i + 1] == '\n' || content[i + 1] == '\r')) {
				++i;
				if (content[i] == '\r' && i + 1 < content.size() && content[i + 1] == '\n') ++i;
				flat.push_back(' ');
			} else if (c == '\n' || c == '\r') {
				flat.push_back(' ');
			} else {
				flat.push_back(c);
			}
		}
		// "<target>: <dep> <dep> ..." の ":" を探す。Windows パスは "C:" を
		// 含むので「直後が空白 or 行末」かつ「":" の位置 != 1」を採用条件にする。
		// (位置 1 はドライブレターの ":" なのでスキップ)
		size_t pos = std::string::npos;
		for (size_t i = 0; i < flat.size(); ++i) {
			if (flat[i] != ':') continue;
			if (i == 1) continue; // C: のような drive letter
			// 直後がパス継続文字 ('\\' or '/') ならパス内の ":" (UNC 等) と
			// みなしてスキップ
			if (i + 1 < flat.size() && (flat[i + 1] == '\\' || flat[i + 1] == '/')) {
				// ただし target の終わりも "X.o: \..." のように直後 ' ' or タブが続く
				// 形をとるので、空白で見分ける。ここに来るのは "X:/..." パスの中。
				continue;
			}
			pos = i;
			break;
		}
		if (pos == std::string::npos) return deps;
		std::string body = flat.substr(pos + 1);
		std::string tok;
		for (size_t i = 0; i < body.size(); ++i) {
			char c = body[i];
			if (c == '\\' && i + 1 < body.size() && body[i + 1] == ' ') {
				tok.push_back(' '); ++i;
			} else if (c == ' ' || c == '\t') {
				if (!tok.empty()) { deps.push_back(tok); tok.clear(); }
			} else {
				tok.push_back(c);
			}
		}
		if (!tok.empty()) deps.push_back(tok);
		return deps;
	}
	// 「セット同一性」判定: 前回保存したファイル一覧と今回のリストが完全一致するか
	bool sameFileSet(const StampMap& saved, const std::vector<FileEntry>& current) {
		if (saved.size() != current.size()) return false;
		for (const auto& e : current) {
			if (saved.find(e.srcPath) == saved.end()) return false;
		}
		return true;
	}
	// 各エントリについて、再コンパイルが必要か判定
	std::vector<size_t> planRebuild(const std::vector<FileEntry>& entries,
		const StampMap& saved, bool forceFullBuild) {
		std::vector<size_t> result;
		if (forceFullBuild || !sameFileSet(saved, entries)) {
			// ファイルが追加/削除された場合は全再コンパイル
			result.reserve(entries.size());
			for (size_t i = 0; i < entries.size(); ++i) result.push_back(i);
			return result;
		}
		for (size_t i = 0; i < entries.size(); ++i) {
			const auto& e = entries[i];
			auto it = saved.find(e.srcPath);
			bool dirty = (it == saved.end())
				|| it->second.mtime != e.mtime
				|| it->second.size != e.size
				|| !Utils::fileExists(e.objPath);
			// ----- ヘッダ依存判定 (.d ファイル経由) -----
			if (!dirty) {
				std::string depPath = e.objPath + ".d";
				if (!Utils::fileExists(depPath)) {
					dirty = true; // .d 未生成 → 安全側に倒す
				} else {
					long long objMtime = Utils::getFileLastWriteTime(e.objPath);
					auto deps = readDepFile(depPath);
					for (const auto& dep : deps) {
						if (dep.empty() || dep == e.srcPath) continue;
						long long depMtime = Utils::getFileLastWriteTime(dep);
						if (depMtime == 0 || depMtime > objMtime) { dirty = true; break; }
					}
				}
			}
			if (dirty) result.push_back(i);
		}
		return result;
	}

	// =============================================================================
	// エラーメッセージ整形
	// =============================================================================

	// gcc 診断行: <file>:<line>:<col>: <kind>: <message>
	// kind = "error" / "fatal error" / "warning" / "note"
	//
	// 処理方針:
	//   1. <file>:<line>:<col>: の部分を太字+白で強調
	//   2. error/fatal error → 赤、warning → 黄、note → シアン
	//   3. メッセージ本文を解析して日本語ヒントを付ける
	//   4. ソース引用行 (数字 | コード) / カーソル行 (^ ~) はそのまま出力
	// =============================================================================

	// エラーメッセージから日本語の補足ヒントを返す
	std::string diagnosticHint(const std::string& msg) {
		// ヘッダが見つからない
		if (msg.find("No such file or directory") != std::string::npos) {
			// #include "foo.h" -> ファイル名を抽出して表示
			std::string header;
			auto q1 = msg.find('\'');
			auto q2 = (q1 != std::string::npos) ? msg.find('\'', q1 + 1) : std::string::npos;
			if (q1 != std::string::npos && q2 != std::string::npos)
				header = msg.substr(q1 + 1, q2 - q1 - 1);
			if (header.empty()) {
				auto b1 = msg.rfind('/'); auto b2 = msg.rfind('\\');
				auto sep = (b1 == std::string::npos) ? b2
					: (b2 == std::string::npos) ? b1 : std::max(b1, b2);
				if (sep != std::string::npos) header = msg.substr(sep + 1);
			}
			std::string hint = "  \033[36m→ ヘッダファイルが見つかりません";
			if (!header.empty()) hint += ": " + header;
			hint += "\n    スケッチフォルダ内に配置されているか確認してください。\033[0m";
			return hint;
		}
		// 未定義の識別子
		if (msg.find("was not declared") != std::string::npos ||
			msg.find("undeclared identifier") != std::string::npos ||
			msg.find("'") != std::string::npos && msg.find("' was not declared") != std::string::npos) {
			return "  \033[36m→ 変数や関数の名前が宣言されていません。スペルミスや #include の漏れを確認してください。\033[0m";
		}
		// 型の不一致
		if (msg.find("cannot convert") != std::string::npos ||
			msg.find("invalid conversion") != std::string::npos) {
			return "  \033[36m→ 型が合っていません。キャストや変数の型を確認してください。\033[0m";
		}
		// セミコロン/括弧忘れ
		if (msg.find("expected ';'") != std::string::npos ||
			msg.find("expected ')'") != std::string::npos ||
			msg.find("expected '}'") != std::string::npos) {
			return "  \033[36m→ 記号の書き忘れがあります。直前の行末を確認してください。\033[0m";
		}
		// 未使用変数 (warning)
		if (msg.find("unused variable") != std::string::npos ||
			msg.find("unused parameter") != std::string::npos) {
			return "  \033[36m→ 宣言したが使っていない変数があります。不要なら削除してください。\033[0m";
		}
		// 多重定義
		if (msg.find("redefinition of") != std::string::npos ||
			msg.find("redeclared") != std::string::npos) {
			return "  \033[36m→ 同名の変数や関数が二重に定義されています。\033[0m";
		}
		// 関数の引数の数が違う
		if (msg.find("too many arguments") != std::string::npos ||
			msg.find("too few arguments") != std::string::npos) {
			return "  \033[36m→ 関数に渡す引数の数が間違っています。\033[0m";
		}
		return "";
	}
	// gcc の診断1行を整形して返す
	// 診断でなければそのまま (ソース引用行・カーソル行など)
	std::string formatDiagnosticLine(const std::string& raw) {
		// フォーマット: path:line:col: kind: message
		// Windows パスは "C:\..." で始まるため最初の ':' はドライブ区切りの可能性あり
		// -> 2番目以降の ": " を区切りとして探す
		size_t colonPos = std::string::npos;
		{
			size_t start = 0;
			// ドライブレター対応: 1文字目が英字で2文字目が ':' ならスキップ
			if (raw.size() >= 2 && std::isalpha((unsigned char)raw[0]) && raw[1] == ':')
				start = 2;
			// file:line:col: の最後の ": " を探す
			// 簡易判定: 3個以上の ':' を持つ行が診断行
			int colons = 0;
			for (size_t i = start; i < raw.size(); ++i) {
				if (raw[i] == ':') {
					++colons;
					if (colons >= 3) { colonPos = i; break; }
				}
			}
		}
		if (colonPos == std::string::npos || colonPos + 2 >= raw.size())
			return raw + "\n";
		// kind 判定
		std::string rest = raw.substr(colonPos + 1);
		// rest の先頭空白をスキップ
		size_t ks = 0;
		while (ks < rest.size() && rest[ks] == ' ') ++ks;
		rest = rest.substr(ks);
		bool isFatal = rest.rfind("fatal error:", 0) == 0;
		bool isError = isFatal || rest.rfind("error:", 0) == 0;
		bool isWarning = !isError && rest.rfind("warning:", 0) == 0;
		bool isNote = !isError && !isWarning && rest.rfind("note:", 0) == 0;
		if (!isError && !isWarning && !isNote) return raw + "\n";
		// location 部分 (path:line:col) を太字・白で強調
		std::string location = raw.substr(0, colonPos);
		// kindとメッセージ部分を色付け
		const char* kindColor = isError ? "\033[1;31m" : isWarning ? "\033[1;33m" : "\033[1;32m";
		const char* reset = "\033[0m";
		const char* bold = "\033[1;37m";
		// メッセージ本体 (kind: の後ろ)
		std::string msgBody;
		{
			auto kc = rest.find(':');
			if (kc != std::string::npos && kc + 1 < rest.size())
				msgBody = rest.substr(kc + 1);
			// 先頭空白除去
			size_t ms = 0;
			while (ms < msgBody.size() && msgBody[ms] == ' ') ++ms;
			msgBody = msgBody.substr(ms);
		}
		// location を "file:line:col" に分解して行番号だけ黄色で強調
		// Windows ドライブレター ("C:") は1文字+':' なので先にスキップ
		std::string locFile, locLine, locCol;
		{
			size_t lstart = 0;
			if (location.size() >= 2 && std::isalpha((unsigned char)location[0]) && location[1] == ':')
				lstart = 2;
			size_t c1 = location.find(':', lstart);  // file と line の間
			size_t c2 = (c1 != std::string::npos) ? location.find(':', c1 + 1) : std::string::npos;
			if (c1 != std::string::npos) {
				locFile = location.substr(0, c1);
				locLine = (c2 != std::string::npos)
					? location.substr(c1 + 1, c2 - c1 - 1)
					: location.substr(c1 + 1);
				if (c2 != std::string::npos) locCol = location.substr(c2 + 1);
			} else {
				locFile = location;
			}
		}
		const char* lineColor = "\033[1;96m"; // シアン太字: 行番号
		std::ostringstream out;
		// ファイル名(白太字) : 行番号(黄太字) : 列(白太字) : kind: message
		out << bold << locFile << reset << ":";
		if (!locLine.empty()) out << lineColor << locLine << reset << ":";
		if (!locCol.empty())  out << bold << locCol << reset << ":";
		out << " " << kindColor << rest.substr(0, rest.find(':') + 1) << reset
			<< " " << msgBody << "\n";  // メッセージ
		return out.str();
	}
	// コンパイラ出力全体を整形する
	// エラー・警告件数のサマリーも末尾に付ける
	std::string colorizeOutput(const std::string& output) {
		std::istringstream iss(output);
		std::ostringstream oss;
		std::string line;
		while (std::getline(iss, line)) {
			// \r 除去
			if (!line.empty() && line.back() == '\r') line.pop_back();
			// カーソル行 (^ ~~~) と ソース引用行は色なしでそのまま
			bool isCaret = true;
			for (char c : line) {
				if (c != '^' && c != '~' && c != ' ' && c != '\t') { isCaret = false; break; }
			}
			if (isCaret && !line.empty()) {
				// カーソル行: ^ の部分だけ赤で強調
				std::ostringstream cl;
				for (char c : line) {
					if (c == '^' || c == '~') cl << "\033[1;31m" << c << "\033[0m";
					else cl << c;
				}
				oss << cl.str() << "\n";
				continue;
			}
			std::string formatted = formatDiagnosticLine(line);
			oss << formatted;
		}
		return oss.str();
	}

	// =============================================================================
	// 実コンパイル/リンク/objcopy
	// =============================================================================

	// コンパイル毎に再生成するのは無駄なので 1 度だけ作って使い回す。
	// .ino だけのスケッチなら 1 ファイルしかコンパイルしないので恩恵は薄いが、
	// 並列ビルド時にスレッド間で文字列を共有 (const 参照) できる利点もある。
	struct CompileFlagsCache {
		std::string cpp;   // 共通の C++ オプション列
		std::string cc;    // 共通の C オプション列
	};

	CompileFlagsCache buildFlagsCache(const BoardConfig& bc,
		const std::string& sketchDir, const std::string& buildDir) {
		CompileFlagsCache c;
		c.cpp = cppFlags(bc, sketchDir, buildDir);
		c.cc  = cFlags(bc, sketchDir, buildDir);
		return c;
	}

	bool compileOne(const FileEntry& e, const CompileFlagsCache& flags,
		std::string& outputLog) {
		// 依存ヘッダ情報を <obj>.d に出力させる (差分判定でヘッダ更新を検出するため)
		//   -MMD: ユーザヘッダのみを依存として出力
		//   -MF : 出力先を明示 (buildDir に入らないのを防ぐ)
		const std::string& flagStr = e.isCpp ? flags.cpp : flags.cc;
		std::string depPath = e.objPath + ".d";
		// 結合は += で連鎖。中間 ostringstream を避ける。
		std::string args;
		args.reserve(flagStr.size() + e.srcPath.size() + e.objPath.size() + depPath.size() + 64);
		args += flagStr;
		args += " -MMD -MF \"";
		args += depPath;
		args += "\" \"";
		args += e.srcPath;
		args += "\" -o \"";
		args += e.objPath;
		args += "\"";
		const std::string& compiler = e.isCpp ? g_state.toolchain.avrGpp : g_state.toolchain.avrGcc;
		auto pr = runProcess(compiler, args, "", true);
		outputLog += colorizeOutput(pr.output);
		if (pr.exitCode != 0) {
			outputLog += colorizeOutput(pr.error);
			return false;
		}
		return true;
	}
	bool runLink(const std::vector<FileEntry>& all, const BoardConfig& bc,
		const std::string& coreA, const std::string& elfPath, std::string& outputLog) {
		std::ostringstream args;
		args << "-w -Os -flto -fuse-linker-plugin -Wl,--gc-sections"
			<< " -mmcu=" << bc.mcu
			<< " -o \"" << elfPath << "\"";
		for (const auto& e : all) args << " \"" << e.objPath << "\"";
		args << " \"" << coreA << "\" -lm";
		auto pr = runProcess(g_state.toolchain.avrGcc, args.str(), "", true);
		outputLog += pr.output;
		if (pr.exitCode != 0) { outputLog += pr.error; return false; }
		return true;
	}
	bool runObjcopy(const std::string& elfPath, const std::string& hexPath,
		std::string& outputLog) {
		std::string args = "-O ihex -R .eeprom \"" + elfPath + "\" \"" + hexPath + "\"";
		auto pr = runProcess(g_state.toolchain.avrObjcopy, args, "", true);
		outputLog += pr.output;
		if (pr.exitCode != 0) { outputLog += pr.error; return false; }
		return true;
	}
} // anonymous namespace

// =============================================================================

// public API

// =============================================================================

namespace Builder {
	CompileResult compile(const CompileRequest& req) {
		static bool ansiReady = [] {
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
			HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
			DWORD mode = 0;
			if (GetConsoleMode(hOut, &mode))
				SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
			if (GetConsoleMode(hErr, &mode))
				SetConsoleMode(hErr, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
			return true;
			}();
		CompileResult out;
		Stopwatch totalSw;
		// --- 入口チェック ---
		if (!g_state.toolchain.valid) {
			out.errorMessage = "Toolchain not initialized: " + g_state.toolchain.errorMessage;
			return out;
		}
		std::string sketchDir = resolveSketchDir(req.sketchDir);
		if (sketchDir.empty() || !Utils::directoryExists(sketchDir)) {
			out.errorMessage = "Cannot resolve sketch directory: \"" + req.sketchDir + "\"";
			return out;
		}
		std::string workspace = resolveWorkspace(sketchDir, req.workspaceDir);
		std::string buildDir = computeBuildDir(workspace, sketchDir);
		Utils::createDirectory(buildDir);
		BoardConfig bc = resolveBoardConfig(req.fqbn);
		// ソース列挙と.ino存在確認を1回のディレクトリ走査で済ませる。
		bool hasIno = false;
		auto entries = collectSources(sketchDir, buildDir, &hasIno);
		if (!hasIno) {
			out.errorMessage = "No .ino file found in: " + sketchDir;
			return out;
		}
		// --- core.a の確保 (初回のみ) ---
		std::string coreA = coreArchivePath(coreCacheKey(bc));
		if (req.forceFullBuild) Utils::deleteFile(coreA);
		{
			std::string err;
			if (!ensureCoreArchive(bc, sketchDir, coreA, err)) {
				out.errorMessage = err;
				return out;
			}
		}
		// --- 差分判定 ---
		// コンパイラフラグやツールチェーンが変わった場合は、mtimeが同じでも
		// 旧オブジェクトを再利用しない。通常時は短い署名ファイルを読むだけ。
		CompileFlagsCache flagsCache = buildFlagsCache(bc, sketchDir, buildDir);
		std::string buildSignature = Utils::sha1Hex(
			flagsCache.cpp + "\n" + flagsCache.cc + "\n" +
			g_state.toolchain.compilerVersion + "\n" + coreA);
		std::string signaturePath = Utils::joinPath(buildDir, ".build-signature");
		bool buildConfigChanged = !Utils::fileExists(signaturePath)
			|| Utils::trim(Utils::readFile(signaturePath)) != buildSignature;
		auto saved = loadStamps(buildDir);
		auto plan = planRebuild(entries, saved,
			req.forceFullBuild || buildConfigChanged);
		std::string sketchName = Utils::getFileName(sketchDir);
		std::string elfPath = Utils::joinPath(buildDir, sketchName + ".elf");
		std::string hexPath = Utils::joinPath(buildDir, sketchName + ".hex");
		out.totalFiles = (int)entries.size();
		out.recompiledFiles = (int)plan.size();
		// --- 完全キャッシュヒット: .hex と .elf があり、変更ゼロ ---
		if (plan.empty() && Utils::fileExists(hexPath) && Utils::fileExists(elfPath)) {
			out.success = true;
			out.cached = true;
			out.hexFile = hexPath;
			out.elfFile = elfPath;
			out.buildTimeMs = totalSw.elapsedMilliseconds();
			// state にも反映 (upload で参照される)
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto& s = g_state.sketches[sketchDir];
			s.sketchDir = sketchDir;
			s.buildDir = buildDir;
			s.hexFile = hexPath;
			s.elfFile = elfPath;
			s.hasValidBuild = true;
			return out;
		}
		// --- コンパイル (差分のみ) ---
		if (plan.size() == 1) {
			// 単一ファイル: スレッド生成のオーバーヘッドを避けて素直に実行
			size_t idx = plan[0];
			if (!compileOne(entries[idx], flagsCache, out.compilerOutput)) {
				out.errorMessage = "Compile failed: "
					+ Utils::getFileName(entries[idx].srcPath);
				return out;
			}
		} else if (!plan.empty()) {
			// 複数ファイル: ハードウェア並列度まで std::async で投げる。
			// 各タスクは自前の outputLog バッファに書き、最後にメインがまとめる
			// (lock 不要)。最初に失敗したファイルのエラーを優先表示。
			unsigned hw = std::thread::hardware_concurrency();
			if (hw == 0) hw = 2;
			// 同時走行数の上限。avr-g++ は I/O+CPU 混合なので物理コア数で十分。
			// メモリ消費 (各 g++ ~100MB) も考慮して 8 に絞る。
			unsigned maxJobs = std::min<unsigned>(hw, 8);

			std::vector<std::string> perLog(plan.size());
			std::vector<int> perOk(plan.size(), -1);  // -1=未実行, 0=失敗, 1=成功
			std::atomic<size_t> nextIdx{ 0 };
			std::atomic<bool> aborted{ false };

			auto worker = [&]() {
				while (!aborted.load(std::memory_order_relaxed)) {
					size_t i = nextIdx.fetch_add(1, std::memory_order_relaxed);
					if (i >= plan.size()) break;
					bool ok = compileOne(entries[plan[i]], flagsCache, perLog[i]);
					perOk[i] = ok ? 1 : 0;
					if (!ok) {
						// 失敗は他のジョブを中断: 残ジョブを早期スキップ
						aborted.store(true, std::memory_order_relaxed);
						break;
					}
				}
			};

			unsigned spawn = std::min<unsigned>(maxJobs, (unsigned)plan.size());
			std::vector<std::future<void>> futs;
			futs.reserve(spawn);
			for (unsigned t = 0; t < spawn; ++t) {
				futs.emplace_back(std::async(std::launch::async, worker));
			}
			for (auto& f : futs) f.get();

			// 出力順序を保つためインデックス順で append
			for (size_t i = 0; i < plan.size(); ++i) {
				if (!perLog[i].empty()) out.compilerOutput += perLog[i];
			}
			// 最初に失敗したファイルのエラーを返す
			for (size_t i = 0; i < plan.size(); ++i) {
				if (perOk[i] == 0) {
					out.errorMessage = "Compile failed: "
						+ Utils::getFileName(entries[plan[i]].srcPath);
					return out;
				}
			}
		}
		// --- 何か再コンパイルしたか、elf/hex が無いならリンク ---
		bool needLink = !plan.empty()
			|| !Utils::fileExists(elfPath)
			|| !Utils::fileExists(hexPath);
		if (needLink) {
			if (!runLink(entries, bc, coreA, elfPath, out.compilerOutput)) {
				out.errorMessage = "Link failed";
				return out;
			}
			if (!runObjcopy(elfPath, hexPath, out.compilerOutput)) {
				out.errorMessage = "objcopy failed";
				return out;
			}
		}
		// --- スタンプ更新 ---
		saveStamps(buildDir, entries);
		Utils::writeFileIfChanged(signaturePath, buildSignature + "\n");
		// --- state 更新 ---
		{
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto& s = g_state.sketches[sketchDir];
			s.sketchDir = sketchDir;
			s.buildDir = buildDir;
			s.hexFile = hexPath;
			s.elfFile = elfPath;
			s.hasValidBuild = true;
			s.lastBuildAt = std::chrono::steady_clock::now();
		}
		out.success = true;
		out.cached = false;
		out.hexFile = hexPath;
		out.elfFile = elfPath;
		out.buildTimeMs = totalSw.elapsedMilliseconds();
		return out;
	}
	UploadResult upload(const UploadRequest& req) {
		UploadResult out;
		std::string sketchDir = resolveSketchDir(req.sketchDir);
		if (sketchDir.empty()) {
			out.errorMessage = "Cannot resolve sketch directory: \"" + req.sketchDir + "\"";
			return out;
		}
		// --- コンパイル ---
		if (!req.skipCompile) {
			CompileRequest cr;
			cr.sketchDir = sketchDir;
			cr.workspaceDir = req.workspaceDir;
			out.compile = compile(cr);
			if (!out.compile.success) {
				out.errorMessage = out.compile.errorMessage;
				return out;
			}
		} else {
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto it = g_state.sketches.find(sketchDir);
			if (it == g_state.sketches.end() || !it->second.hasValidBuild) {
				out.errorMessage = "No prior build to upload";
				return out;
			}
			out.compile.success = true;
			out.compile.cached = true;
			out.compile.hexFile = it->second.hexFile;
			out.compile.elfFile = it->second.elfFile;
		}
		// --- ポート決定 ---
		std::string port = req.port;
		if (port.empty()) {
			std::lock_guard<std::mutex> lk(g_state.portMtx);
			port = g_state.cachedArduinoPort;
			if (port.empty() && !g_state.cachedPorts.empty()) {
				port = g_state.cachedPorts.front();
			}
		}
		if (port.empty()) {
			out.errorMessage = "No COM port detected";
			return out;
		}
		out.port = port;

		// =====================================================================

		// ネイティブ STK500v2 アップロード (avrdude を使わない)

		// =====================================================================

		std::shared_ptr<const std::vector<uint8_t>> flash;
		long long hexMtime = 0;
		uint64_t hexSize = 0;
		Utils::getFileMetadata(out.compile.hexFile, hexMtime, hexSize);
		bool flashCacheHit = false;
		{
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto it = g_state.sketches.find(sketchDir);
			if (it != g_state.sketches.end()
				&& it->second.flashImage
				&& it->second.flashHexMtime == hexMtime
				&& it->second.flashHexSize == hexSize) {
				flash = it->second.flashImage;
				flashCacheHit = true;
			}
		}
		if (!flash) {
			auto parsed = std::make_shared<std::vector<uint8_t>>();
			std::string hexErr;
			if (!Stk500v2::readIntelHex(out.compile.hexFile, *parsed, hexErr)) {
				out.errorMessage = "Hex parse failed: " + hexErr;
				return out;
			}
			flash = parsed;
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto& state = g_state.sketches[sketchDir];
			state.flashImage = parsed;
			state.flashHexMtime = hexMtime;
			state.flashHexSize = hexSize;
		}
		std::shared_ptr<const std::vector<uint8_t>> previousFlash;
		if (!req.forceFullUpload) {
			std::lock_guard<std::mutex> lk(g_state.uploadMtx);
			auto it = g_state.uploadedFlashByPort.find(port);
			if (it != g_state.uploadedFlashByPort.end()) {
				previousFlash = it->second;
			}
		}
		if (previousFlash && *previousFlash == *flash) {
			out.success = true;
			out.cached = true;
			out.uploadTimeMs = 0;
			out.avrdudeOutput = "upload cache hit: identical flash image\n";
			return out;
		}
		// 注: 旧 warmupComPort は削除。SerialPort::open() が直後に同じ COM を開くため重複していた。
		auto stats = Stk500v2::uploadMega2560(port, *flash,
			previousFlash ? previousFlash.get() : nullptr);
		// デバッグ用に内訳もログに残す
		std::ostringstream oss;
		oss << "stk500v2 native upload\n"
			<< "  open=" << stats.openMs << "ms"
			<< " reset=" << stats.resetMs << "ms"
			<< " sync=" << stats.syncMs << "ms"
			<< " prog=" << stats.progMs << "ms"
			<< " leave=" << stats.leaveMs << "ms\n"
			<< "  pages=" << stats.pagesWritten
			<< " skipped=" << stats.pagesSkipped
			<< " bytes=" << stats.bytesWritten
			<< " flash_cache=" << (flashCacheHit ? "hit" : "miss")
			<< " delta_cache=" << (previousFlash ? "hit" : "miss") << "\n";
		out.avrdudeOutput = oss.str();
		if (!stats.success) {
			out.errorMessage = stats.errorMessage;
			return out;
		}
		{
			std::lock_guard<std::mutex> lk(g_state.uploadMtx);
			g_state.uploadedFlashByPort[port] = flash;
		}
		out.success = true;
		out.uploadTimeMs = stats.totalMs;
		return out;
	}

	// =============================================================================

	// JSON シリアライズ

	// =============================================================================

	nlohmann::json toJson(const CompileResult& r) {
		return {
			{"success", r.success},
			{"cached", r.cached},
			{"recompiledFiles", r.recompiledFiles},
			{"totalFiles", r.totalFiles},
			{"hexFile", r.hexFile},
			{"elfFile", r.elfFile},
			{"buildTimeMs", r.buildTimeMs},
			{"errorMessage", r.errorMessage},
			{"compilerOutput", r.compilerOutput}
		};
	}
	nlohmann::json toJson(const UploadResult& r) {
		return {
			{"success", r.success},
			{"cached", r.cached},
			{"port", r.port},
			{"uploadTimeMs", r.uploadTimeMs},
			{"errorMessage", r.errorMessage},
			{"avrdudeOutput", r.avrdudeOutput},
			{"compile", toJson(r.compile)}
		};
	}
} // namespace Builder
