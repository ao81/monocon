#include "stk500v2.h"
#include "builder.h"
#include "daemon_state.h"
#include "utils.h"
#include "port_scanner.h"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

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

	// .ino / フォルダ / 末尾スラッシュ を吸収して「.ino を含むディレクトリ」を返す
	std::string resolveSketchDir(const std::string& raw) {
		std::error_code ec;
		fs::path p(raw);

		if (p.extension() == ".ino") {
			if (fs::is_regular_file(p, ec)) return p.parent_path().string();
			return "";
		}
		if (fs::is_directory(p, ec)) {
			for (const auto& entry : fs::directory_iterator(p, ec)) {
				if (ec) break;
				if (entry.path().extension() == ".ino") return p.string();
			}
			return p.string();   // .ino 無し: エラーは呼び出し元で
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

	// <workspace>/.vscode/build/<sketchDir からの相対パス>
	std::string computeBuildDir(const std::string& workspace, const std::string& sketch) {
		std::error_code ec;
		fs::path ws = fs::absolute(workspace, ec);
		fs::path sd = fs::absolute(sketch, ec);
		fs::path rel = fs::relative(sd, ws, ec);
		std::string r = rel.string();
		bool inside = !ec && !r.empty() && r != "." && r.rfind("..", 0) != 0;

		fs::path base = ws / ".vscode" / "build";
		if (inside) return (base / rel).string();

		std::string leaf = sd.filename().string();
		if (leaf.empty()) leaf = "sketch";
		return (base / leaf).string();
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
		f << "-c -g -Os -Wall -Wextra -std=gnu++17"
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
		f << "-c -g -Os -Wall -Wextra -std=gnu11"
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

	std::string coreArchivePath(const std::string& key) {
		std::string dir = Utils::joinPath(g_state.coreCacheRoot, key);
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

		std::string tmpBuild = Utils::joinPath(Utils::getGlobalCacheDir(), "tmp-build");
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

	// 全 .ino を 1 つの .cpp に結合してファイルに書き出す
	//   内容が前回と同じなら mtime を変えないので、差分判定が誤動作しない
	std::string mergeInoFiles(const std::vector<std::string>& inos,
		const std::string& sketchDir,
		const std::string& buildDir) {
		std::string sketchName = Utils::getFileName(sketchDir);
		std::string outPath = Utils::joinPath(buildDir, sketchName + ".ino.cpp");

		std::ostringstream out;
		out << "// Auto-generated by arduino-build-daemon\n"
			<< "#include <Arduino.h>\n";
		for (size_t i = 0; i < inos.size(); ++i) {
			out << "#line 1 \"" << Utils::getFileName(inos[i]) << "\"\n"
				<< Utils::readFile(inos[i]) << "\n";
		}
		std::string content = out.str();
		Utils::writeFileIfChanged(outPath, content);
		return outPath;
	}

	// =============================================================================
	// ソース列挙とプラン作成
	// =============================================================================

	FileEntry makeEntry(const std::string& src, const std::string& obj, bool isCpp) {
		FileEntry e;
		e.srcPath = src;
		e.objPath = obj;
		e.isCpp = isCpp;
		e.mtime = Utils::getFileLastWriteTime(src);
		std::error_code ec;
		e.size = fs::file_size(src, ec);
		if (ec) e.size = 0;
		return e;
	}

	// すべてのコンパイル単位を列挙
	std::vector<FileEntry> collectSources(const std::string& sketchDir,
		const std::string& buildDir) {
		std::vector<FileEntry> entries;

		// 1) .ino を結合した 1 ファイル
		auto inos = Utils::getFilesByExtension(sketchDir, ".ino");
		std::sort(inos.begin(), inos.end());
		if (!inos.empty()) {
			std::string merged = mergeInoFiles(inos, sketchDir, buildDir);
			FileEntry e;
			e.srcPath = merged;
			e.objPath = merged + ".o";
			e.isCpp = true;
			e.mtime = 0;
			e.size = 0;
			for (const auto& ino : inos) {
				long long t = Utils::getFileLastWriteTime(ino);
				if (t > e.mtime) e.mtime = t;
				std::error_code ec;
				auto s = fs::file_size(ino, ec);
				if (!ec) e.size += s;
			}
			entries.push_back(e);
		}
		// 2) .cpp
		for (const auto& src : Utils::getFilesByExtension(sketchDir, ".cpp")) {
			std::string obj = Utils::joinPath(buildDir, Utils::getFileStem(src) + ".cpp.o");
			entries.push_back(makeEntry(src, obj, true));
		}
		// 3) .c
		for (const auto& src : Utils::getFilesByExtension(sketchDir, ".c")) {
			std::string obj = Utils::joinPath(buildDir, Utils::getFileStem(src) + ".c.o");
			entries.push_back(makeEntry(src, obj, false));
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
			if (dirty) result.push_back(i);
		}
		return result;
	}

	std::string colorizeOutput(const std::string& output) {
		std::istringstream iss(output);
		std::ostringstream oss;
		std::string line;
		while (std::getline(iss, line)) {
			if (line.find("error:") != std::string::npos) {
				oss << "\033[31m" << line << "\033[0m\n"; // 赤
			} else if (line.find("warning:") != std::string::npos) {
				oss << "\033[33m" << line << "\033[0m\n"; // 黄
			} else {
				oss << line << "\n";
			}
		}
		return oss.str();
	}

	// =============================================================================
	// 実コンパイル/リンク/objcopy
	// =============================================================================

	bool compileOne(const FileEntry& e, const BoardConfig& bc,
		const std::string& sketchDir, const std::string& buildDir,
		std::string& outputLog) {
		std::string flags = e.isCpp
			? cppFlags(bc, sketchDir, buildDir)
			: cFlags(bc, sketchDir, buildDir);
		std::string args = flags + " \"" + e.srcPath + "\" -o \"" + e.objPath + "\"";
		std::string compiler = e.isCpp ? g_state.toolchain.avrGpp : g_state.toolchain.avrGcc;

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
		args << "-w -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections"
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

	// COMポートを一瞬開閉して、ドライバの初期化遅延を取り除く
	// avrdudeが直後に開く際の数十〜100msのオーバーヘッドが消える
	void warmupComPort(const std::string& port) {
		std::string devName = "\\\\.\\" + port;
		HANDLE h = CreateFileA(devName.c_str(),
			GENERIC_READ | GENERIC_WRITE, 0, nullptr,
			OPEN_EXISTING, 0, nullptr);
		if (h != INVALID_HANDLE_VALUE) {
			CloseHandle(h);
		}
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
		if (Utils::getFilesByExtension(sketchDir, ".ino").empty()) {
			out.errorMessage = "No .ino file found in: " + sketchDir;
			return out;
		}

		std::string workspace = resolveWorkspace(sketchDir, req.workspaceDir);
		std::string buildDir = computeBuildDir(workspace, sketchDir);
		Utils::createDirectory(buildDir);

		BoardConfig bc = resolveBoardConfig(req.fqbn);

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

		// --- ソース列挙 + 差分判定 ---
		auto entries = collectSources(sketchDir, buildDir);
		auto saved = loadStamps(buildDir);
		auto plan = planRebuild(entries, saved, req.forceFullBuild);

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
		for (size_t idx : plan) {
			if (!compileOne(entries[idx], bc, sketchDir, buildDir, out.compilerOutput)) {
				out.errorMessage = "Compile failed: "
					+ Utils::getFileName(entries[idx].srcPath);
				return out;
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
		Stopwatch sw;

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
		std::vector<uint8_t> flash;
		std::string hexErr;
		if (!Stk500v2::readIntelHex(out.compile.hexFile, flash, hexErr)) {
			out.errorMessage = "Hex parse failed: " + hexErr;
			return out;
		}

		// COMポートウォームアップ
		warmupComPort(port);

		auto stats = Stk500v2::uploadMega2560(port, flash);

		// デバッグ用に内訳もログに残す
		std::ostringstream oss;
		oss << "stk500v2 native upload\n"
			<< "  open=" << stats.openMs << "ms"
			<< " reset=" << stats.resetMs << "ms"
			<< " sync=" << stats.syncMs << "ms"
			<< " prog=" << stats.progMs << "ms"
			<< " leave=" << stats.leaveMs << "ms\n"
			<< "  pages=" << stats.pagesWritten
			<< " bytes=" << stats.bytesWritten << "\n";
		out.avrdudeOutput = oss.str();

		if (!stats.success) {
			out.errorMessage = stats.errorMessage;
			return out;
		}

		out.success = true;
		out.uploadTimeMs = sw.elapsedMilliseconds();
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
			{"port", r.port},
			{"uploadTimeMs", r.uploadTimeMs},
			{"errorMessage", r.errorMessage},
			{"avrdudeOutput", r.avrdudeOutput},
			{"compile", toJson(r.compile)}
		};
	}

} // namespace Builder