#include "builder.h"
#include "daemon_state.h"
#include "utils.h"
#include "port_scanner.h"

#include <windows.h>
#include <sstream>
#include <set>

namespace {

	// ---------------------------------------------------------------------
	// fqbn から MEGA/ADK の MCU 設定を引く
	// ---------------------------------------------------------------------
	struct BoardConfig {
		std::string mcu;     // "atmega2560"
		std::string fcpu;    // "16000000L"
		std::string variant; // "mega"
		std::string programmer; // "wiring"
		long uploadBaud;     // 115200
	};

	BoardConfig resolveBoardConfig(const std::string& fqbn) {
		BoardConfig c;
		// デフォルトは MEGA 2560
		c.mcu = "atmega2560";
		c.fcpu = "16000000L";
		c.variant = "mega";
		c.programmer = "wiring";
		c.uploadBaud = 115200;

		// "arduino:avr:mega:cpu=atmega2560"   → MEGA 2560
		// "arduino:avr:megaADK"               → MEGA ADK (atmega2560)
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

	// ---------------------------------------------------------------------
	// fqbn / flags から core.a のキャッシュキーを作る
	// ---------------------------------------------------------------------
	std::string buildCoreCacheKey(const BoardConfig& bc) {
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

	// ---------------------------------------------------------------------
	// スケッチディレクトリ正規化 (キーとして使うため)
	// ---------------------------------------------------------------------
	std::string normalizeDir(const std::string& dir) {
		std::error_code ec;
		auto abs = std::filesystem::absolute(dir, ec);
		if (ec) return dir;
		std::string s = abs.string();
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char ch) { return (char)std::tolower(ch); });
		return s;
	}

	// ---------------------------------------------------------------------
	// スケッチ + ライブラリの全 .ino/.cpp/.c/.h を列挙して signature を作る
	// ---------------------------------------------------------------------
	std::string computeSourceSignature(const std::string& sketchDir) {
		std::vector<std::string> all;
		for (const auto& ext : { ".ino", ".cpp", ".c", ".h", ".hpp" }) {
			auto files = Utils::getFilesByExtension(sketchDir, ext);
			all.insert(all.end(), files.begin(), files.end());
		}
		std::sort(all.begin(), all.end());
		return Utils::getSourceSignature(all);
	}

	// ---------------------------------------------------------------------
	// arduino-cli を探す (PATH または %LOCALAPPDATA%)
	// ---------------------------------------------------------------------
	std::string findArduinoCli() {
		// 1) PATH
		char buf[MAX_PATH * 2];
		DWORD r = SearchPathA(nullptr, "arduino-cli.exe", nullptr,
			sizeof(buf), buf, nullptr);
		if (r > 0 && r < sizeof(buf)) return std::string(buf, r);

		// 2) よくある場所
		std::vector<std::string> candidates = {
			Utils::joinPath(Utils::getLocalAppDataPath(), "Programs\\arduino-cli\\arduino-cli.exe"),
			"C:\\Program Files\\Arduino CLI\\arduino-cli.exe",
		};
		for (const auto& c : candidates) {
			if (Utils::fileExists(c)) return c;
		}
		return "";
	}

	// ---------------------------------------------------------------------
	// arduino-cli compile を 1 回だけ走らせて core.a を取り出す
	//   → core-cache に保存
	// ---------------------------------------------------------------------
	bool generateCoreArchive(const BoardConfig& bc,
		const std::string& sketchDir,
		const std::string& targetCoreA,
		std::string& errorOut) {
		std::string cli = findArduinoCli();
		if (cli.empty()) {
			errorOut = "arduino-cli not found (cannot generate core.a)";
			return false;
		}

		// 一時ビルドディレクトリ
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
			errorOut = "arduino-cli compile failed:\n" + pr.error + "\n" + pr.output;
			return false;
		}

		// build-path/core/core.a を target にコピー
		std::string srcCoreA = Utils::joinPath(tmpBuild, "core\\core.a");
		if (!Utils::fileExists(srcCoreA)) {
			errorOut = "core.a not produced by arduino-cli at " + srcCoreA;
			return false;
		}
		std::error_code ec;
		std::filesystem::copy_file(srcCoreA, targetCoreA,
			std::filesystem::copy_options::overwrite_existing, ec);
		if (ec) {
			errorOut = "Failed to copy core.a: " + ec.message();
			return false;
		}
		return true;
	}

	// ---------------------------------------------------------------------
	// 共通の avr-g++ / avr-gcc コンパイルフラグ
	// ---------------------------------------------------------------------
	std::string commonDefines(const BoardConfig& bc) {
		std::ostringstream f;
		f << " -DF_CPU=" << bc.fcpu;
		f << " -DARDUINO=10819";
		f << " -DARDUINO_AVR_MEGA2560";
		f << " -DARDUINO_ARCH_AVR";
		return f.str();
	}

	std::string includeFlags(const std::string& sketchDir,
		const std::string& buildDir = "") {
		std::ostringstream f;
		// コアヘッダ
		f << " -I\"" << g_state.toolchain.coreDir << "\"";
		// ボードバリアント (mega 等)
		if (!g_state.toolchain.variantDir.empty()) {
			f << " -I\"" << g_state.toolchain.variantDir << "\"";
		}
		// スケッチディレクトリ自身 (mono_con.h など同階層のヘッダを解決)
		if (!sketchDir.empty()) {
			f << " -I\"" << sketchDir << "\"";
		}
		// ビルドディレクトリ (.ino を .cpp に変換したファイルのヘッダ解決)
		if (!buildDir.empty()) {
			f << " -I\"" << buildDir << "\"";
		}
		return f.str();
	}

	std::string cppFlags(const BoardConfig& bc,
		const std::string& sketchDir,
		const std::string& buildDir = "") {
		std::ostringstream f;
		f << "-c -g -Os -Wall -Wextra -std=gnu++17"
			<< " -ffunction-sections -fdata-sections -fno-exceptions"
			<< " -fno-threadsafe-statics -Wno-error=narrowing"
			<< " -mmcu=" << bc.mcu
			<< commonDefines(bc)
			<< includeFlags(sketchDir, buildDir);
		return f.str();
	}

	std::string cFlags(const BoardConfig& bc,
		const std::string& sketchDir,
		const std::string& buildDir = "") {
		std::ostringstream f;
		f << "-c -g -Os -Wall -Wextra -std=gnu11"
			<< " -ffunction-sections -fdata-sections"
			<< " -mmcu=" << bc.mcu
			<< commonDefines(bc)
			<< includeFlags(sketchDir, buildDir);
		return f.str();
	}

	// ---------------------------------------------------------------------
	// .ino を .cpp に「ラップ」する: 先頭で Arduino.h を include して
	// プロトタイプ宣言を補う。本格的なプリプロセスは arduino-cli に任せ、
	// インクリメンタル用の簡易版だけ実装する。
	// ---------------------------------------------------------------------
	std::string wrapInoToCpp(const std::string& inoPath) {
		std::string content = Utils::readFile(inoPath);
		std::ostringstream out;
		out << "// Auto-generated by arduino-build-daemon\n";
		out << "#include <Arduino.h>\n";
		out << "#line 1 \"" << Utils::getFileName(inoPath) << "\"\n";
		out << content;
		return out.str();
	}

	// ---------------------------------------------------------------------
	// インクリメンタルコンパイル本体
	// ---------------------------------------------------------------------
	bool incrementalCompile(const Builder::CompileRequest& req,
		const BoardConfig& bc,
		const std::string& coreA,
		Builder::CompileResult& out,
		SketchBuildState& s) {
		Stopwatch sw;

		std::string sketchName = Utils::getFileName(s.sketchDir);
		// buildDir は呼び出し元 (compile()) で既に s.buildDir に設定済み
		const std::string& buildDir = s.buildDir;
		Utils::createDirectory(buildDir);

		// 1) .ino を 1 つの .cpp に統合 (簡易: 最初の .ino だけ)
		auto inos = Utils::getFilesByExtension(s.sketchDir, ".ino");
		std::vector<std::string> objects;

		auto compileOne = [&](const std::string& src, const std::string& obj,
			bool isCpp) -> bool {
				// sketchDir と buildDir を両方 -I に加える
				std::string flags = isCpp
					? cppFlags(bc, s.sketchDir, buildDir)
					: cFlags(bc, s.sketchDir, buildDir);
				std::string args = flags + " \"" + src + "\" -o \"" + obj + "\"";
				std::string compiler = isCpp ? g_state.toolchain.avrGpp : g_state.toolchain.avrGcc;
				auto pr = runProcess(compiler, args, "", true);
				if (pr.exitCode != 0) {
					out.compilerOutput += pr.error + pr.output;
					return false;
				}
				out.compilerOutput += pr.output;
				return true;
			};

		// .ino を結合して main.cpp として 1 つコンパイル
		if (!inos.empty()) {
			std::string mergedSrc = Utils::joinPath(buildDir, sketchName + ".ino.cpp");
			std::string merged;
			for (size_t i = 0; i < inos.size(); ++i) {
				if (i == 0) merged += wrapInoToCpp(inos[i]);
				else merged += "\n#line 1 \"" + Utils::getFileName(inos[i]) + "\"\n"
					+ Utils::readFile(inos[i]);
			}
			Utils::writeFile(mergedSrc, merged);
			std::string obj = mergedSrc + ".o";
			if (!compileOne(mergedSrc, obj, true)) {
				out.errorMessage = "Compile failed: " + Utils::getFileName(inos[0]);
				return false;
			}
			objects.push_back(obj);
		}

		// 2) 追加の .cpp / .c
		for (const auto& src : Utils::getFilesByExtension(s.sketchDir, ".cpp")) {
			std::string obj = Utils::joinPath(buildDir,
				Utils::getFileStem(src) + ".cpp.o");
			if (!compileOne(src, obj, true)) {
				out.errorMessage = "Compile failed: " + Utils::getFileName(src);
				return false;
			}
			objects.push_back(obj);
		}
		for (const auto& src : Utils::getFilesByExtension(s.sketchDir, ".c")) {
			std::string obj = Utils::joinPath(buildDir,
				Utils::getFileStem(src) + ".c.o");
			if (!compileOne(src, obj, false)) {
				out.errorMessage = "Compile failed: " + Utils::getFileName(src);
				return false;
			}
			objects.push_back(obj);
		}

		// 3) リンク (avr-gcc 経由)
		std::string elfFile = Utils::joinPath(buildDir, sketchName + ".elf");
		std::ostringstream linkArgs;
		linkArgs << "-w -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections"
			<< " -mmcu=" << bc.mcu
			<< " -o \"" << elfFile << "\"";
		for (const auto& o : objects) linkArgs << " \"" << o << "\"";
		linkArgs << " \"" << coreA << "\" -lm";

		auto linkRes = runProcess(g_state.toolchain.avrGcc, linkArgs.str(), "", true);
		if (linkRes.exitCode != 0) {
			out.compilerOutput += linkRes.error + linkRes.output;
			out.errorMessage = "Link failed";
			return false;
		}

		// 4) objcopy で .hex 生成
		std::string hexFile = Utils::joinPath(buildDir, sketchName + ".hex");
		std::string objArgs = "-O ihex -R .eeprom \"" + elfFile + "\" \"" + hexFile + "\"";
		auto objRes = runProcess(g_state.toolchain.avrObjcopy, objArgs, "", true);
		if (objRes.exitCode != 0) {
			out.compilerOutput += objRes.error + objRes.output;
			out.errorMessage = "objcopy failed";
			return false;
		}

		out.elfFile = elfFile;
		out.hexFile = hexFile;
		out.buildTimeMs = sw.elapsedMilliseconds();
		out.success = true;
		s.elfFile = elfFile;
		s.hexFile = hexFile;
		s.hasValidBuild = true;
		s.lastBuildAt = std::chrono::steady_clock::now();
		return true;
	}

} // namespace

namespace {

	// ---------------------------------------------------------------------
	// スケッチディレクトリを正規化する
	//
	// 受け取るパスは以下のいずれかの形式になりうる:
	//   (a) "C:\projects\MySketch"           ← ディレクトリ直指定
	//   (b) "C:\projects\MySketch\MySketch.ino" ← .ino ファイル直指定
	//   (c) "C:\projects\MySketch\"          ← 末尾スラッシュ付き
	//
	// → いずれも「.ino を含むディレクトリ」のパスを返す。
	// 解決できなければ空文字を返す。
	// ---------------------------------------------------------------------
	std::string resolveSketchDir(const std::string& raw) {
		namespace fs = std::filesystem;
		std::error_code ec;

		fs::path p = fs::path(raw);

		// (b) .ino ファイルが直接渡された場合 → 親ディレクトリを使う
		if (p.extension() == ".ino") {
			if (fs::is_regular_file(p, ec)) {
				return p.parent_path().string();
			}
			// .ino パスだが存在しない → エラー
			return "";
		}

		// (a)(c) ディレクトリが渡された場合
		if (fs::is_directory(p, ec)) {
			// ディレクトリ内に .ino が存在するか確認
			for (const auto& entry : fs::directory_iterator(p, ec)) {
				if (ec) break;
				if (entry.path().extension() == ".ino") {
					return p.string();
				}
			}
			// .ino が見つからない → そのまま返す (エラーは呼び出し元で検出)
			return p.string();
		}

		return "";
	}

	// ---------------------------------------------------------------------
	// ワークスペースルート解決
	//
	// hint が指定されていればそれを使う。
	// 未指定なら sketchDir から親ディレクトリを辿って .vscode/ を探す。
	// 見つからなければ sketchDir 自身を返す (フォールバック)。
	// ---------------------------------------------------------------------
	std::string resolveWorkspaceRoot(const std::string& sketchDir,
		const std::string& hint) {
		namespace fs = std::filesystem;
		std::error_code ec;

		// 1) ヒントが妥当ならそれを使う
		if (!hint.empty()) {
			fs::path h = fs::absolute(hint, ec);
			if (!ec && fs::is_directory(h, ec)) {
				return h.string();
			}
		}

		// 2) sketchDir から上方向に .vscode/ を探す
		fs::path p = fs::absolute(sketchDir, ec);
		if (ec) return sketchDir;
		while (!p.empty()) {
			std::error_code ec2;
			if (fs::is_directory(p / ".vscode", ec2)) {
				return p.string();
			}
			fs::path parent = p.parent_path();
			if (parent == p) break;  // ドライブルートに到達
			p = parent;
		}

		// 3) フォールバック: sketchDir 自身をワークスペース扱い
		return sketchDir;
	}

	// ---------------------------------------------------------------------
	// ビルドディレクトリの計算
	//
	//   <workspace>/.vscode/build/<workspace からの相対パス>
	//
	// 例:
	//   workspace = C:\projects\foo
	//   sketchDir = C:\projects\foo\practice\1
	//   → C:\projects\foo\.vscode\build\practice\1
	//
	// sketch が workspace 外なら、sketch のフォルダ名でフォールバック。
	// ---------------------------------------------------------------------
	std::string computeBuildDir(const std::string& workspaceDir,
		const std::string& sketchDir) {
		namespace fs = std::filesystem;
		std::error_code ec;
		fs::path ws = fs::absolute(workspaceDir, ec);
		fs::path sd = fs::absolute(sketchDir, ec);

		fs::path rel = fs::relative(sd, ws, ec);
		std::string relStr = rel.string();

		// 同一 (rel == ".") か、workspace 内 (".." で始まらない) の場合
		bool insideWorkspace = !ec
			&& !relStr.empty()
			&& relStr != "."
			&& relStr.rfind("..", 0) != 0;

		fs::path buildBase = ws / ".vscode" / "build";
		if (insideWorkspace) {
			return (buildBase / rel).string();
		}
		// workspace 外 / 同一 → スケッチ名で
		std::string leaf = sd.filename().string();
		if (leaf.empty()) leaf = "sketch";
		return (buildBase / leaf).string();
	}

} // anonymous namespace (builder-local)

namespace Builder {

	// ---------------------------------------------------------------------
	// public: compile
	// ---------------------------------------------------------------------
	CompileResult compile(const CompileRequest& req) {
		CompileResult out;
		Stopwatch totalSw;

		if (!g_state.toolchain.valid) {
			out.errorMessage = "Toolchain not initialized: " + g_state.toolchain.errorMessage;
			return out;
		}

		// --- スケッチディレクトリの正規化 ---
		// .ino ファイルパス / ディレクトリパス / 末尾スラッシュ付き のいずれも吸収する
		std::string sketchDir = resolveSketchDir(req.sketchDir);
		if (sketchDir.empty()) {
			out.errorMessage =
				"Cannot resolve sketch directory from: \"" + req.sketchDir + "\"\n"
				"  .ino ファイルのパス、またはそれを含むフォルダを指定してください。";
			return out;
		}
		if (!Utils::directoryExists(sketchDir)) {
			out.errorMessage = "Sketch directory does not exist: " + sketchDir;
			return out;
		}
		// .ino が存在するか
		auto inos = Utils::getFilesByExtension(sketchDir, ".ino");
		if (inos.empty()) {
			out.errorMessage =
				"No .ino file found in: " + sketchDir + "\n"
				"  tasks.json の args を ${fileDirname} に設定してください。";
			return out;
		}

		// --- ワークスペースルートとビルドディレクトリの決定 ---
		// 例:
		//   sketchDir    = C:\projects\foo\practice\1
		//   workspaceDir = C:\projects\foo
		//   buildDir     = C:\projects\foo\.vscode\build\practice\1
		std::string workspaceDir = resolveWorkspaceRoot(sketchDir, req.workspaceDir);
		std::string buildDir = computeBuildDir(workspaceDir, sketchDir);
		Utils::createDirectory(buildDir);

		BoardConfig bc = resolveBoardConfig(req.fqbn);
		std::string key = buildCoreCacheKey(bc);
		std::string coreA = coreArchivePath(key);

		// 1) core.a 用意 (なければ arduino-cli で 1 回生成)
		if (!Utils::fileExists(coreA) || req.forceFullBuild) {
			std::string errBuf;
			// arduino-cli には正規化済みのディレクトリパスを渡す
			if (!generateCoreArchive(bc, sketchDir, coreA, errBuf)) {
				out.errorMessage = errBuf;
				return out;
			}
		}

		// 2) スケッチ状態取得 (キーは正規化済みパス)
		std::string sketchKey = normalizeDir(sketchDir);
		SketchBuildState* s = nullptr;
		{
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			s = &g_state.sketches[sketchKey];
			s->sketchDir = sketchDir;
			s->buildDir = buildDir;   // incrementalCompile に伝える
		}

		// 3) インクリメンタルチェック
		std::string newSig = computeSourceSignature(sketchDir);
		if (!req.forceFullBuild
			&& s->hasValidBuild
			&& s->lastSignature == newSig
			&& Utils::fileExists(s->hexFile)) {
			out.success = true;
			out.cached = true;
			out.hexFile = s->hexFile;
			out.elfFile = s->elfFile;
			out.buildTimeMs = totalSw.elapsedMilliseconds();
			return out;
		}

		// 4) 実ビルド (正規化済み sketchDir を持つ request を使う)
		CompileRequest normalizedReq = req;
		normalizedReq.sketchDir = sketchDir;
		if (!incrementalCompile(normalizedReq, bc, coreA, out, *s)) return out;

		s->lastSignature = newSig;
		out.buildTimeMs = totalSw.elapsedMilliseconds();
		return out;
	}

	// ---------------------------------------------------------------------
	// public: upload
	// ---------------------------------------------------------------------
	UploadResult upload(const UploadRequest& req) {
		UploadResult out;
		Stopwatch sw;

		// sketchDir を正規化 (compile() と同じルールで)
		std::string sketchDir = resolveSketchDir(req.sketchDir);
		if (sketchDir.empty()) {
			out.errorMessage = "Cannot resolve sketch directory from: \"" + req.sketchDir + "\"";
			return out;
		}

		// 1) コンパイル (skipCompile が false なら必ず実行)
		if (!req.skipCompile) {
			CompileRequest cr;
			cr.sketchDir = sketchDir;          // 正規化済みを渡す
			cr.workspaceDir = req.workspaceDir; // ワークスペースヒントを伝播
			out.compile = compile(cr);
			if (!out.compile.success) {
				out.errorMessage = out.compile.errorMessage;
				return out;
			}
		} else {
			// スキップ時は最新ビルド結果を取得
			std::string key = normalizeDir(sketchDir);
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto it = g_state.sketches.find(key);
			if (it == g_state.sketches.end() || !it->second.hasValidBuild) {
				out.errorMessage = "No prior build to upload";
				return out;
			}
			out.compile.success = true;
			out.compile.cached = true;
			out.compile.hexFile = it->second.hexFile;
			out.compile.elfFile = it->second.elfFile;
		}

		// 2) ポート決定
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

		// 3) avrdude 実行
		BoardConfig bc = resolveBoardConfig("");
		std::ostringstream args;
		args << "-C\"" << g_state.toolchain.avrdudeConf << "\""
			<< " -v -p" << bc.mcu
			<< " -c" << bc.programmer
			<< " -P" << port
			<< " -b" << bc.uploadBaud
			<< " -D"
			<< " -Uflash:w:\"" << out.compile.hexFile << "\":i";

		auto pr = runProcess(g_state.toolchain.avrdude, args.str(), "", true);
		out.avrdudeOutput = pr.output + pr.error;
		if (pr.exitCode != 0) {
			out.errorMessage = "avrdude failed (exit " + std::to_string(pr.exitCode) + ")";
			return out;
		}

		out.success = true;
		out.uploadTimeMs = sw.elapsedMilliseconds();
		return out;
	}

	// ---------------------------------------------------------------------
	// JSON シリアライズ
	// ---------------------------------------------------------------------
	nlohmann::json toJson(const CompileResult& r) {
		return {
			{"success", r.success},
			{"cached", r.cached},
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