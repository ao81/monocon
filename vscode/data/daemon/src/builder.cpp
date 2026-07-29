#include "stk500v2.h"
#include "builder.h"
#include "daemon_state.h"
#include "utils.h"
#include "port_scanner.h"
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <future>
#include <iterator>
#include <limits>
#include <set>
#include <sstream>
#include <thread>
#include <unordered_set>
namespace fs = std::filesystem;

// =============================================================================

// 全体の処理フロー (要約)
//
//   compile(req)
//     ├ resolveSketch()     : .ino ファイルパス / ディレクトリパスを正規化
//     ├ ensureCoreArchive() : core.a が無ければ arduino-cli で 1 回生成
//     ├ collectSources()    : .ino と src/・ライブラリの C/C++/ASM を列挙
//     ├ loadStamps()        : 前回のソース内容署名を .stamps から読む
//     ├ planRebuild()       : 変更ファイルを特定 (差分判定の核心)
//     ├ runCompiles()       : 変更ファイルだけ avr-g++/avr-gcc を呼ぶ
//     ├ runLink()           : 全 .o + core.a を avr-gcc でリンク
//     ├ runObjcopy()        : .elf → .hex
//     └ saveStamps()        : .stamps を更新
//
// 設計原則:
//   - すべてのデータは「型」として明示する (ぼやっとした文字列は使わない)
//   - 各フェーズは 1 つの関数として独立し、副作用を局所化する
//   - キャッシュはソース、依存、オブジェクト、成果物の内容署名で検証する

// =============================================================================

namespace {

	// =============================================================================

	// データ型

	// =============================================================================

	class NamedMutexGuard {
	public:
		explicit NamedMutexGuard(const std::string& key,
			DWORD timeoutMs = 300000) {
			const std::string keyHash = Utils::sha1Hex(key);
			if (keyHash.size() < 32) {
				error_ = "Cannot hash named mutex key";
				return;
			}
			const std::string name = "Local\\Monocon_" +
				keyHash.substr(0, 32);
			handle_ = CreateMutexW(nullptr, FALSE,
				Utils::utf8ToWide(name).c_str());
			if (!handle_) {
				error_ = "CreateMutex failed: " +
					std::to_string(GetLastError());
				return;
			}
			const DWORD waitResult = WaitForSingleObject(handle_, timeoutMs);
			acquired_ = waitResult == WAIT_OBJECT_0
				|| waitResult == WAIT_ABANDONED;
			if (!acquired_) {
				error_ = waitResult == WAIT_TIMEOUT
					? "Timed out waiting for another build"
					: "WaitForSingleObject failed: " +
						std::to_string(GetLastError());
			}
		}
		~NamedMutexGuard() {
			if (acquired_) ReleaseMutex(handle_);
			if (handle_) CloseHandle(handle_);
		}
		NamedMutexGuard(const NamedMutexGuard&) = delete;
		NamedMutexGuard& operator=(const NamedMutexGuard&) = delete;

		bool acquired() const { return acquired_; }
		const std::string& error() const { return error_; }

	private:
		HANDLE handle_ = nullptr;
		bool acquired_ = false;
		std::string error_;
	};

	class ScopedDirectoryCleanup {
	public:
		explicit ScopedDirectoryCleanup(std::vector<fs::path> paths)
			: paths_(std::move(paths)) {
		}
		~ScopedDirectoryCleanup() {
			for (const auto& path : paths_) {
				std::error_code ec;
				fs::remove_all(path, ec);
			}
		}
		ScopedDirectoryCleanup(const ScopedDirectoryCleanup&) = delete;
		ScopedDirectoryCleanup& operator=(const ScopedDirectoryCleanup&) = delete;
	private:
		std::vector<fs::path> paths_;
	};

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
		bool isAssembler = false; // avr-gcc -x assembler-with-cpp
		long long mtime = 0;
		uintmax_t size = 0;
		std::string contentHash;   // mtime を信用せず内容でキャッシュを検証
	};

	// AVR向けとして現実的な範囲を十分上回る一方、誤って巨大な動画や
	// 生成物ツリーをsrc/へ置いた場合にワーカーがメモリ・ディスクを
	// 使い切らないための入力上限。
	constexpr size_t MAX_SKETCH_FILE_COUNT = 4096;
	constexpr uint64_t MAX_SINGLE_SKETCH_FILE_BYTES = 32ull * 1024 * 1024;
	constexpr uint64_t MAX_TOTAL_SKETCH_FILE_BYTES = 128ull * 1024 * 1024;
	constexpr uint64_t MAX_TOTAL_SKETCH_TEXT_BYTES = 64ull * 1024 * 1024;
	constexpr uint64_t MAX_COMPILED_HEX_BYTES = 4ull * 1024 * 1024;
	constexpr size_t MAX_CACHED_SKETCHES = 64;
	constexpr auto MAX_BUILD_CACHE_AGE = std::chrono::hours(24 * 30);
	constexpr auto MIN_BUILD_CACHE_DELETE_IDLE = std::chrono::hours(1);

	// =============================================================================

	// パス/環境ユーティリティ

	// =============================================================================

	// ポータブル版VSCodeのキャッシュディレクトリを特定
	const std::string& getPortableCacheDir() {
		if (!g_state.buildCacheRoot.empty()) return g_state.buildCacheRoot;
		// 実行ファイル位置はプロセス中に変化しないため、初回だけ解決する。
		static const std::string cacheDir = [] {
			std::wstring executable(32768, L'\0');
			const DWORD length = GetModuleFileNameW(
				nullptr, executable.data(),
				static_cast<DWORD>(executable.size()));
			if (length == 0 || length >= executable.size()) {
				throw std::runtime_error(
					"Cannot resolve executable path for build cache");
			}
			executable.resize(length);
			fs::path p = executable;

			auto toLower = [](std::wstring s) {
				std::transform(s.begin(), s.end(), s.begin(),
					[](wchar_t c) { return (wchar_t)std::towlower(c); });
				return s;
			};

			while (!p.empty() && p.parent_path() != p) {
				std::wstring name = toLower(p.filename().native());
				if (name.find(L"vscode-win32") != std::wstring::npos || name == L"vscode") {
					return Utils::pathToUtf8(p / L"data" / L"cache");
				}
				p = p.parent_path();
			}
			return Utils::pathToUtf8(
				fs::path(executable).parent_path() / L"data" / L"cache");
			}();
		return cacheDir;
	}

	// .ino / フォルダ / 末尾スラッシュ を吸収して「.ino を含むディレクトリ」を返す
	std::string resolveSketchDir(const std::string& raw) {
		std::error_code ec;
		fs::path p = Utils::pathFromUtf8(raw);
		if (p.empty()) return "";
		const fs::path canonical = fs::weakly_canonical(p, ec);
		if (!ec) {
			p = canonical;
		}
		else {
			ec.clear();
			p = fs::absolute(p, ec).lexically_normal();
			if (ec) return "";
		}
		std::wstring extension = p.extension().native();
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](wchar_t c) { return (wchar_t)std::towlower(c); });
		if (extension == L".ino") {
			if (fs::is_regular_file(p, ec)) return Utils::pathToUtf8(p.parent_path());
			return "";
		}
		if (fs::is_directory(p, ec)) {
			return Utils::pathToUtf8(p);   // .ino 有無は後続の1回のソース走査で確認
		}
		return "";
	}

	bool isValidComPortName(const std::string& port) {
		if (port.size() < 4) return false;
		if (std::toupper(static_cast<unsigned char>(port[0])) != 'C'
			|| std::toupper(static_cast<unsigned char>(port[1])) != 'O'
			|| std::toupper(static_cast<unsigned char>(port[2])) != 'M'
			|| port[3] < '1' || port[3] > '9') {
			return false;
		}
		return std::all_of(port.begin() + 4, port.end(),
			[](unsigned char c) { return std::isdigit(c) != 0; });
	}

	std::string normalizeComPortName(std::string port) {
		std::transform(port.begin(), port.end(), port.begin(),
			[](unsigned char c) {
				return static_cast<char>(std::toupper(c));
			});
		return port;
	}

	std::string requiredFileHash(const std::string& path) {
		std::string hash = Utils::sha1FileHex(path);
		if (hash.empty()) {
			throw std::runtime_error("Cannot hash file: " + path);
		}
		return hash;
	}

	bool trimmedFileEquals(const std::string& path,
		const std::string& expected, size_t maxBytes = 4096) noexcept {
		try {
			return Utils::trim(Utils::readFileLimited(path, maxBytes))
				== expected;
		}
		catch (...) {
			return false;
		}
	}

	// ポータブルディレクトリの cache/build/sketch_<ハッシュ> に保存。
	// AVR GCC 7のWindows版はコマンドラインの非ACP文字を扱えないため、
	// 生成物名はASCIIだけに限定する。
	std::string computeBuildDir(const std::string& sketch) {
		std::error_code ec;
		fs::path cacheRoot = Utils::pathFromUtf8(getPortableCacheDir());
		fs::path sd = fs::absolute(Utils::pathFromUtf8(sketch), ec);

		// 同名のスケッチ名が別ディレクトリにある場合の衝突回避
		std::string canonicalSketch = Utils::pathToUtf8(sd);
		const std::string fullHash = Utils::sha1Hex(canonicalSketch);
		if (fullHash.size() < 16) {
			throw std::runtime_error("Cannot hash sketch cache identity");
		}
		std::string hash = fullHash.substr(0, 16);
		fs::path base = cacheRoot / L"build"
			/ Utils::pathFromUtf8("sketch_" + hash);

		return Utils::pathToUtf8(base);
	}

	bool isManagedBuildCacheName(const fs::path& path) {
		const std::wstring name = path.filename().native();
		if (name.size() != 23 || name.rfind(L"sketch_", 0) != 0) {
			return false;
		}
		return std::all_of(name.begin() + 7, name.end(), [](wchar_t c) {
			return (c >= L'0' && c <= L'9')
				|| (c >= L'a' && c <= L'f');
		});
	}

	void touchBuildCache(const std::string& buildDir) {
		const auto now = std::chrono::system_clock::now().time_since_epoch();
		const auto milliseconds =
			std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
		Utils::writeFile(
			Utils::joinPath(buildDir, ".last-used"),
			std::to_string(milliseconds) + "\n");
	}

	void pruneBuildCache(const std::string& currentBuildDir) {
		// 毎回のキャッシュヒットを重くしない。1プロセスにつき初回と、
		// 以後16ビルドごとにだけ保守する。
		static std::atomic<unsigned> maintenanceTick{ 0 };
		if ((maintenanceTick.fetch_add(1, std::memory_order_relaxed) % 16) != 0) {
			return;
		}
		NamedMutexGuard maintenanceLock("build-cache-maintenance", 0);
		if (!maintenanceLock.acquired()) return;

		const fs::path buildRoot =
			Utils::pathFromUtf8(getPortableCacheDir()) / L"build";
		std::error_code ec;
		if (!fs::is_directory(buildRoot, ec)) return;

		struct Candidate {
			fs::path path;
			fs::file_time_type lastUsed;
		};
		std::vector<Candidate> candidates;
		for (const auto& entry : fs::directory_iterator(
			buildRoot, fs::directory_options::skip_permission_denied, ec)) {
			if (ec) break;
			std::error_code entryEc;
			if (!entry.is_directory(entryEc)
				|| !isManagedBuildCacheName(entry.path())) {
				continue;
			}
			fs::path marker = entry.path() / L".last-used";
			fs::file_time_type lastUsed =
				fs::last_write_time(marker, entryEc);
			if (entryEc) {
				entryEc.clear();
				lastUsed = fs::last_write_time(entry.path(), entryEc);
			}
			if (!entryEc) candidates.push_back({ entry.path(), lastUsed });
		}
		std::sort(candidates.begin(), candidates.end(),
			[](const Candidate& a, const Candidate& b) {
				return a.lastUsed < b.lastUsed;
			});

		size_t remaining = candidates.size();
		const auto now = fs::file_time_type::clock::now();
		for (const auto& candidate : candidates) {
			if (remaining <= MAX_CACHED_SKETCHES
				&& now - candidate.lastUsed <= MAX_BUILD_CACHE_AGE) {
				break;
			}
			if (Utils::pathToUtf8(candidate.path) == currentBuildDir
				|| now - candidate.lastUsed < MIN_BUILD_CACHE_DELETE_IDLE) {
				continue;
			}

			const std::string candidatePath =
				Utils::pathToUtf8(candidate.path);
			NamedMutexGuard buildLock("build:" + candidatePath, 0);
			if (!buildLock.acquired()) continue;

			// 候補はdirectory_iterator由来かつ厳格な名前形式だが、削除直前
			// にもう一度親を照合し、キャッシュルート外を消さない。
			if (candidate.path.parent_path() != buildRoot
				|| !isManagedBuildCacheName(candidate.path)) {
				continue;
			}
			std::error_code removeEc;
			const uintmax_t removed = fs::remove_all(candidate.path, removeEc);
			if (!removeEc && removed > 0) --remaining;
		}
	}

	void maintainBuildCache(const std::string& currentBuildDir) noexcept {
		// 保守メタデータを書けないことを、正常なコンパイル失敗へ波及させない。
		try {
			touchBuildCache(currentBuildDir);
			pruneBuildCache(currentBuildDir);
		}
		catch (...) {
		}
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
	std::string includeFlags(const std::string& sketchDir,
		const std::string& buildDir,
		const std::vector<std::string>& libraryIncludeDirs) {
		std::ostringstream f;
		f << " -I\"" << Utils::getShortPath(g_state.toolchain.coreDir) << "\"";
		if (!g_state.toolchain.variantDir.empty()) {
			f << " -I\"" << Utils::getShortPath(
				g_state.toolchain.variantDir) << "\"";
		}
		const std::string stagedDir = Utils::joinPath(buildDir, "staged");
		f << " -I\"" << Utils::getShortPath(stagedDir) << "\""
			<< " -I\"" << Utils::getShortPath(sketchDir) << "\""
			<< " -I\"" << Utils::getShortPath(buildDir) << "\"";
		for (const auto& includeDir : libraryIncludeDirs) {
			f << " -I\"" << Utils::getShortPath(includeDir) << "\"";
		}
		return f.str();
	}
	std::string cppFlags(const BoardConfig& bc, const std::string& sketchDir,
		const std::string& buildDir,
		const std::vector<std::string>& libraryIncludeDirs) {
		std::ostringstream f;
		f << "-c -Os -pipe -Wall -Wextra -std=gnu++11"
			<< " -fpermissive -fno-exceptions"
			<< " -ffunction-sections -fdata-sections"
			<< " -fno-threadsafe-statics -Wno-error=narrowing"
			<< " -Wno-unused-variable -flto"
			<< " -mmcu=" << bc.mcu
			<< commonDefines(bc)
			<< includeFlags(sketchDir, buildDir, libraryIncludeDirs);
		return f.str();
	}
	std::string cFlags(const BoardConfig& bc, const std::string& sketchDir,
		const std::string& buildDir,
		const std::vector<std::string>& libraryIncludeDirs) {
		std::ostringstream f;
		f << "-c -Os -pipe -Wall -Wextra -std=gnu11"
			<< " -ffunction-sections -fdata-sections"
			<< " -Wno-unused-variable -flto -fno-fat-lto-objects"
			<< " -mmcu=" << bc.mcu
			<< commonDefines(bc)
			<< includeFlags(sketchDir, buildDir, libraryIncludeDirs);
		return f.str();
	}
	std::string assemblerFlags(const BoardConfig& bc,
		const std::string& sketchDir, const std::string& buildDir,
		const std::vector<std::string>& libraryIncludeDirs) {
		std::ostringstream f;
		f << "-c -Os -x assembler-with-cpp"
			<< " -ffunction-sections -fdata-sections -flto"
			<< " -mmcu=" << bc.mcu
			<< commonDefines(bc)
			<< includeFlags(sketchDir, buildDir, libraryIncludeDirs);
		return f.str();
	}

	// =============================================================================

	// core.a キャッシュ

	// =============================================================================

	std::string coreCacheKey(const BoardConfig& bc) {
		std::ostringstream oss;
		oss << bc.mcu << "|" << bc.fcpu << "|" << bc.variant << "|"
			<< g_state.toolchain.compilerVersion << "|"
			<< g_state.toolchain.coreDir << "|"
			<< g_state.toolchain.variantDir << "|"
			<< (g_state.toolchain.prebuiltCoreHash.empty()
				? std::to_string(Utils::getFileLastWriteTime(g_state.toolchain.coreDir))
				: g_state.toolchain.prebuiltCoreHash);
		return Utils::sha1Hex(oss.str()).substr(0, 16);
	}

	// ポータブルディレクトリ内に cores フォルダを構築
	std::string coreArchivePath(const std::string& key) {
		fs::path cacheRoot = Utils::pathFromUtf8(getPortableCacheDir());
		std::string dir = Utils::joinPath(
			Utils::pathToUtf8(cacheRoot / L"cores"), key);
		Utils::createDirectory(dir);
		return Utils::joinPath(dir, "core.a");
	}

	std::string findArduinoCli() {
		wchar_t buf[MAX_PATH * 2];
		DWORD r = SearchPathW(nullptr, L"arduino-cli.exe", nullptr,
			static_cast<DWORD>(std::size(buf)), buf, nullptr);
		if (r > 0 && r < std::size(buf)) {
			return Utils::wideToUtf8(std::wstring(buf, r));
		}
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
	bool ensureCoreArchive(const BoardConfig& bc,
		const std::string& targetCoreA, std::string& errOut) {
		// 複数のVS Codeウィンドウが同時起動しても、共有core.aを読みかけの
		// 状態でリンクしないようプロセス間で直列化する。
		NamedMutexGuard coreLock("core:" + targetCoreA);
		if (!coreLock.acquired()) {
			errOut = "Cannot lock core cache: " + coreLock.error();
			return false;
		}
		const std::string hashPath = targetCoreA + ".sha1";
		if (Utils::fileExists(targetCoreA)) {
			const std::string actualHash = Utils::sha1FileHex(targetCoreA);
			const std::string expectedHash =
				!g_state.toolchain.prebuiltCoreHash.empty()
				? g_state.toolchain.prebuiltCoreHash
				: (Utils::fileExists(hashPath)
					? [&]() {
						try {
							return Utils::trim(
								Utils::readFileLimited(hashPath, 4096));
						}
						catch (...) {
							return std::string();
						}
					}()
					: std::string());
			if (!actualHash.empty() && actualHash == expectedHash) {
				return true;
			}
			// 不完全コピー、ディスク破損、旧版の未検証キャッシュは再生成する。
			Utils::deleteFile(targetCoreA);
			Utils::deleteFile(hashPath);
		}
		if (Utils::fileExists(g_state.toolchain.prebuiltCoreA)) {
			std::error_code ec;
			fs::copy_file(Utils::pathFromUtf8(g_state.toolchain.prebuiltCoreA),
				Utils::pathFromUtf8(targetCoreA),
				fs::copy_options::overwrite_existing, ec);
			if (!ec
				&& Utils::sha1FileHex(targetCoreA)
					== g_state.toolchain.prebuiltCoreHash) {
				Utils::writeFile(
					hashPath, g_state.toolchain.prebuiltCoreHash + "\n");
				return true;
			}
			Utils::deleteFile(targetCoreA);
			errOut = "Failed to install bundled core.a: " + ec.message();
			if (!ec) errOut = "Bundled core.a verification failed after copy";
			return false;
		}
		std::string cli = findArduinoCli();
		if (cli.empty()) {
			errOut = "arduino-cli not found (cannot generate core.a)";
			return false;
		}

		// core.a生成のためにユーザーのスケッチをArduino CLIへ渡すと、
		// Unicodeファイル名を古いAVR GCCが開けず初回だけ失敗する。また、
		// ユーザーコードの構文エラーがcore生成を妨げる。ASCII固定名の最小
		// スケッチを使い、生成後は成功・失敗を問わず一時物を回収する。
		const std::string temporaryKey =
			Utils::sha1Hex(targetCoreA).substr(0, 12);
		const std::string tmpBuild = Utils::joinPath(
			getPortableCacheDir(), "tmp-build-" + temporaryKey);
		const std::string tmpSketch = Utils::joinPath(
			getPortableCacheDir(), "tmp-core-sketch-" + temporaryKey);
		const fs::path tmpBuildPath = Utils::pathFromUtf8(tmpBuild);
		const fs::path tmpSketchPath = Utils::pathFromUtf8(tmpSketch);
		for (const auto& path : { tmpBuildPath, tmpSketchPath }) {
			std::error_code cleanupEc;
			fs::remove_all(path, cleanupEc);
			if (cleanupEc) {
				errOut = "Cannot clean temporary core directory: "
					+ cleanupEc.message();
				return false;
			}
		}
		Utils::createDirectory(tmpBuild);
		Utils::createDirectory(tmpSketch);
		ScopedDirectoryCleanup temporaryCleanup({
			tmpBuildPath, tmpSketchPath
		});
		const std::string probeIno =
			Utils::joinPath(tmpSketch, "tmp-core-sketch-" + temporaryKey + ".ino");
		Utils::writeFile(probeIno,
			"void setup() {}\nvoid loop() {}\n");

		std::string fqbn = (bc.variant == "megaADK")
			? "arduino:avr:megaADK"
			: "arduino:avr:mega:cpu=" + bc.mcu;
		std::ostringstream args;
		args << "compile --fqbn " << fqbn
			<< " --build-path \"" << tmpBuild << "\""
			<< " \"" << tmpSketch << "\"";
		auto pr = runProcess(cli, args.str(), "", true, 300000);
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
		fs::copy_file(Utils::pathFromUtf8(srcCoreA),
			Utils::pathFromUtf8(targetCoreA),
			fs::copy_options::overwrite_existing, ec);
		if (ec) {
			errOut = "Failed to copy core.a: " + ec.message();
			return false;
		}
		const std::string generatedHash = Utils::sha1FileHex(targetCoreA);
		if (generatedHash.empty()) {
			Utils::deleteFile(targetCoreA);
			errOut = "Generated core.a verification failed";
			return false;
		}
		Utils::writeFile(hashPath, generatedHash + "\n");
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

	struct FunctionDefinition {
		size_t signatureStart = 0;
		std::string prototype;
		std::vector<std::pair<size_t, size_t>> defaultRanges;
	};

	std::string trimCopy(const std::string& value) {
		size_t first = value.find_first_not_of(" \t\r\n");
		if (first == std::string::npos) return "";
		size_t last = value.find_last_not_of(" \t\r\n");
		return value.substr(first, last - first + 1);
	}

	bool isIdentifierChar(char c);

	class PreprocessorExpression {
	public:
		PreprocessorExpression(const std::string& expression,
			const std::unordered_map<std::string, std::string>& macros,
			const std::unordered_map<std::string, std::string>& functionMacros,
			std::unordered_set<std::string> expanding = {})
			: expression_(expression), macros_(macros),
			functionMacros_(functionMacros),
			expanding_(std::move(expanding)) {
		}

		long long evaluate() {
			position_ = 0;
			const long long value = parseConditional();
			skipWhitespace();
			if (position_ < expression_.size()) reliable_ = false;
			return value;
		}
		bool reliable() const { return reliable_; }

	private:
		static constexpr size_t MAX_EXPRESSION_RECURSION = 256;
		static constexpr size_t MAX_MACRO_EXPANSION_DEPTH = 64;

		void skipWhitespace() {
			while (position_ < expression_.size()
				&& std::isspace(static_cast<unsigned char>(
					expression_[position_]))) {
				++position_;
			}
		}

		bool consume(const char* token) {
			skipWhitespace();
			const size_t length = std::char_traits<char>::length(token);
			if (expression_.compare(position_, length, token) != 0) {
				return false;
			}
			position_ += length;
			return true;
		}

		std::string parseIdentifier() {
			skipWhitespace();
			const size_t start = position_;
			if (position_ >= expression_.size()
				|| !(std::isalpha(static_cast<unsigned char>(
					expression_[position_]))
					|| expression_[position_] == '_')) {
				return "";
			}
			++position_;
			while (position_ < expression_.size()
				&& isIdentifierChar(expression_[position_])) {
				++position_;
			}
			return expression_.substr(start, position_ - start);
		}

		long long macroValue(const std::string& name) {
			auto it = macros_.find(name);
			if (it == macros_.end() || expanding_.find(name) != expanding_.end()) {
				return 0;
			}
			if (expanding_.size() >= MAX_MACRO_EXPANSION_DEPTH) {
				reliable_ = false;
				return 0;
			}
			auto nestedExpanding = expanding_;
			nestedExpanding.insert(name);
			PreprocessorExpression nested(
				it->second.empty() ? "1" : it->second,
				macros_, functionMacros_, std::move(nestedExpanding));
			const long long value = nested.evaluate();
			if (!nested.reliable()) reliable_ = false;
			return value;
		}

		long long parseNumber() {
			skipWhitespace();
			const size_t start = position_;
			if (position_ + 2 <= expression_.size()
				&& expression_[position_] == '0'
				&& position_ + 1 < expression_.size()
				&& (expression_[position_ + 1] == 'b'
					|| expression_[position_ + 1] == 'B')) {
				position_ += 2;
				unsigned long long value = 0;
				while (position_ < expression_.size()
					&& (expression_[position_] == '0'
						|| expression_[position_] == '1')) {
					value = value * 2
						+ static_cast<unsigned>(expression_[position_] - '0');
					++position_;
				}
				while (position_ < expression_.size()
					&& (expression_[position_] == 'u'
						|| expression_[position_] == 'U'
						|| expression_[position_] == 'l'
						|| expression_[position_] == 'L')) {
					++position_;
				}
				return static_cast<long long>(value);
			}
			char* end = nullptr;
			const unsigned long long value = std::strtoull(
				expression_.c_str() + position_, &end, 0);
			if (end == expression_.c_str() + position_) {
				return 0;
			}
			position_ = static_cast<size_t>(end - expression_.c_str());
			while (position_ < expression_.size()
				&& (expression_[position_] == 'u'
					|| expression_[position_] == 'U'
					|| expression_[position_] == 'l'
					|| expression_[position_] == 'L')) {
				++position_;
			}
			(void)start;
			return static_cast<long long>(value);
		}

		long long parseCharacter() {
			skipWhitespace();
			if (position_ >= expression_.size()
				|| expression_[position_] != '\'') {
				return 0;
			}
			++position_;
			long long value = 0;
			if (position_ < expression_.size()
				&& expression_[position_] == '\\') {
				++position_;
				if (position_ < expression_.size()) {
					const char escaped = expression_[position_++];
					switch (escaped) {
					case 'n': value = '\n'; break;
					case 'r': value = '\r'; break;
					case 't': value = '\t'; break;
					case '0': value = '\0'; break;
					default: value = static_cast<unsigned char>(escaped); break;
					}
				}
			}
			else if (position_ < expression_.size()) {
				value = static_cast<unsigned char>(expression_[position_++]);
			}
			while (position_ < expression_.size()
				&& expression_[position_] != '\'') {
				++position_;
			}
			if (position_ < expression_.size()) ++position_;
			return value;
		}

		bool parseInvocationArguments(std::vector<std::string>& arguments) {
			skipWhitespace();
			if (position_ >= expression_.size()
				|| expression_[position_] != '(') {
				return false;
			}
			++position_;
			size_t argumentStart = position_;
			int depth = 1;
			char quote = '\0';
			bool escaped = false;
			while (position_ < expression_.size()) {
				const char c = expression_[position_];
				if (quote != '\0') {
					if (escaped) escaped = false;
					else if (c == '\\') escaped = true;
					else if (c == quote) quote = '\0';
					++position_;
					continue;
				}
				if (c == '\'' || c == '"') {
					quote = c;
					++position_;
					continue;
				}
				if (c == '(') {
					++depth;
				}
				else if (c == ')') {
					--depth;
					if (depth == 0) {
						arguments.push_back(trimCopy(expression_.substr(
							argumentStart, position_ - argumentStart)));
						++position_;
						if (arguments.size() == 1 && arguments[0].empty()) {
							arguments.clear();
						}
						return true;
					}
				}
				else if (c == ',' && depth == 1) {
					arguments.push_back(trimCopy(expression_.substr(
						argumentStart, position_ - argumentStart)));
					argumentStart = position_ + 1;
				}
				++position_;
			}
			reliable_ = false;
			return false;
		}

		long long functionMacroValue(const std::string& name) {
			auto definition = functionMacros_.find(name);
			if (definition == functionMacros_.end()
				|| expanding_.find(name) != expanding_.end()) {
				std::vector<std::string> ignored;
				parseInvocationArguments(ignored);
				reliable_ = false;
				return 0;
			}
			if (expanding_.size() >= MAX_MACRO_EXPANSION_DEPTH) {
				std::vector<std::string> ignored;
				parseInvocationArguments(ignored);
				reliable_ = false;
				return 0;
			}
			std::vector<std::string> arguments;
			if (!parseInvocationArguments(arguments)) return 0;

			const std::string& raw = definition->second;
			if (raw.empty() || raw[0] != '(') {
				reliable_ = false;
				return 0;
			}
			size_t close = raw.find(')');
			if (close == std::string::npos) {
				reliable_ = false;
				return 0;
			}
			std::vector<std::string> parameters;
			std::string parameterList = raw.substr(1, close - 1);
			size_t start = 0;
			while (start <= parameterList.size()) {
				size_t comma = parameterList.find(',', start);
				if (comma == std::string::npos) comma = parameterList.size();
				const std::string parameter = trimCopy(
					parameterList.substr(start, comma - start));
				if (!parameter.empty()) parameters.push_back(parameter);
				if (comma == parameterList.size()) break;
				start = comma + 1;
			}
			if (parameters.size() != arguments.size()) {
				reliable_ = false;
				return 0;
			}
			std::unordered_map<std::string, std::string> replacements;
			for (size_t i = 0; i < parameters.size(); ++i) {
				replacements[parameters[i]] = "(" + arguments[i] + ")";
			}
			const std::string body = trimCopy(raw.substr(close + 1));
			std::string expanded;
			for (size_t i = 0; i < body.size();) {
				if (std::isalpha(static_cast<unsigned char>(body[i]))
					|| body[i] == '_') {
					size_t end = i + 1;
					while (end < body.size() && isIdentifierChar(body[end])) {
						++end;
					}
					const std::string token = body.substr(i, end - i);
					auto replacement = replacements.find(token);
					expanded += replacement == replacements.end()
						? token : replacement->second;
					i = end;
				}
				else {
					expanded.push_back(body[i++]);
				}
			}
			auto nestedExpanding = expanding_;
			nestedExpanding.insert(name);
			PreprocessorExpression nested(expanded, macros_, functionMacros_,
				std::move(nestedExpanding));
			const long long value = nested.evaluate();
			if (!nested.reliable()) reliable_ = false;
			return value;
		}

		long long parsePrimary() {
			skipWhitespace();
			if (consume("(")) {
				if (recursionDepth_ >= MAX_EXPRESSION_RECURSION) {
					reliable_ = false;
					return 0;
				}
				++recursionDepth_;
				const long long value = parseConditional();
				--recursionDepth_;
				consume(")");
				return value;
			}
			if (position_ < expression_.size()
				&& expression_[position_] == '\'') {
				return parseCharacter();
			}
			if (position_ < expression_.size()
				&& std::isdigit(static_cast<unsigned char>(
					expression_[position_]))) {
				return parseNumber();
			}
			const std::string identifier = parseIdentifier();
			if (identifier.empty()) {
				if (position_ < expression_.size()) {
					++position_;
					reliable_ = false;
				}
				return 0;
			}
			if (identifier == "defined") {
				skipWhitespace();
				const bool parenthesized = consume("(");
				const std::string name = parseIdentifier();
				if (parenthesized) consume(")");
				return macros_.find(name) != macros_.end()
					|| functionMacros_.find(name) != functionMacros_.end()
					? 1 : 0;
			}
			skipWhitespace();
			if (position_ < expression_.size()
				&& expression_[position_] == '(') {
				return functionMacroValue(identifier);
			}
			return macroValue(identifier);
		}

		long long parseUnary() {
			char unary = '\0';
			if (consume("!")) unary = '!';
			else if (consume("~")) unary = '~';
			else if (consume("+")) unary = '+';
			else if (consume("-")) unary = '-';
			if (unary != '\0') {
				if (recursionDepth_ >= MAX_EXPRESSION_RECURSION) {
					reliable_ = false;
					return 0;
				}
				++recursionDepth_;
				const long long operand = parseUnary();
				--recursionDepth_;
				switch (unary) {
				case '!': return !operand;
				case '~': return ~operand;
				case '+': return operand;
				case '-':
					return static_cast<long long>(
						0ull - static_cast<unsigned long long>(operand));
				default: return operand;
				}
			}
			return parsePrimary();
		}

		long long parseMultiplicative() {
			long long value = parseUnary();
			for (;;) {
				if (consume("*")) {
					value = static_cast<long long>(
						static_cast<unsigned long long>(value)
						* static_cast<unsigned long long>(parseUnary()));
				}
				else if (consume("/")) {
					const long long right = parseUnary();
					if (right == 0) {
						reliable_ = false;
						value = 0;
					}
					else if (value == (std::numeric_limits<long long>::min)()
						&& right == -1) {
						reliable_ = false;
					}
					else value /= right;
				}
				else if (consume("%")) {
					const long long right = parseUnary();
					if (right == 0) {
						reliable_ = false;
						value = 0;
					}
					else if (value == (std::numeric_limits<long long>::min)()
						&& right == -1) {
						reliable_ = false;
						value = 0;
					}
					else value %= right;
				}
				else return value;
			}
		}

		long long parseAdditive() {
			long long value = parseMultiplicative();
			for (;;) {
				if (consume("+")) {
					value = static_cast<long long>(
						static_cast<unsigned long long>(value)
						+ static_cast<unsigned long long>(
							parseMultiplicative()));
				}
				else if (consume("-")) {
					value = static_cast<long long>(
						static_cast<unsigned long long>(value)
						- static_cast<unsigned long long>(
							parseMultiplicative()));
				}
				else return value;
			}
		}

		long long parseShift() {
			long long value = parseAdditive();
			for (;;) {
				if (consume("<<")) {
					const long long shift = parseAdditive();
					if (shift < 0 || shift >= 64) {
						reliable_ = false;
						value = 0;
					}
					else {
						value = static_cast<long long>(
							static_cast<unsigned long long>(value)
							<< static_cast<unsigned>(shift));
					}
				}
				else if (consume(">>")) {
					const long long shift = parseAdditive();
					if (shift < 0 || shift >= 64) {
						reliable_ = false;
						value = 0;
					}
					else value >>= static_cast<unsigned>(shift);
				}
				else return value;
			}
		}

		long long parseRelational() {
			long long value = parseShift();
			for (;;) {
				if (consume("<=")) value = value <= parseShift();
				else if (consume(">=")) value = value >= parseShift();
				else if (consume("<")) value = value < parseShift();
				else if (consume(">")) value = value > parseShift();
				else return value;
			}
		}

		long long parseEquality() {
			long long value = parseRelational();
			for (;;) {
				if (consume("==")) value = value == parseRelational();
				else if (consume("!=")) value = value != parseRelational();
				else return value;
			}
		}

		long long parseBitwiseAnd() {
			long long value = parseEquality();
			for (;;) {
				skipWhitespace();
				if (expression_.compare(position_, 2, "&&") == 0
					|| position_ >= expression_.size()
					|| expression_[position_] != '&') {
					return value;
				}
				++position_;
				value &= parseEquality();
			}
		}

		long long parseBitwiseXor() {
			long long value = parseBitwiseAnd();
			while (consume("^")) value ^= parseBitwiseAnd();
			return value;
		}

		long long parseBitwiseOr() {
			long long value = parseBitwiseXor();
			for (;;) {
				skipWhitespace();
				if (expression_.compare(position_, 2, "||") == 0
					|| position_ >= expression_.size()
					|| expression_[position_] != '|') {
					return value;
				}
				++position_;
				value |= parseBitwiseXor();
			}
		}

		long long parseLogicalAnd() {
			long long value = parseBitwiseOr();
			while (consume("&&")) {
				const long long right = parseBitwiseOr();
				value = value && right;
			}
			return value;
		}

		long long parseLogicalOr() {
			long long value = parseLogicalAnd();
			while (consume("||")) {
				const long long right = parseLogicalAnd();
				value = value || right;
			}
			return value;
		}

		long long parseConditional() {
			const long long condition = parseLogicalOr();
			if (!consume("?")) return condition;
			if (recursionDepth_ >= MAX_EXPRESSION_RECURSION) {
				reliable_ = false;
				return condition;
			}
			++recursionDepth_;
			const long long whenTrue = parseConditional();
			if (!consume(":")) {
				--recursionDepth_;
				reliable_ = false;
				return condition;
			}
			const long long whenFalse = parseConditional();
			--recursionDepth_;
			return condition ? whenTrue : whenFalse;
		}

		std::string expression_;
		const std::unordered_map<std::string, std::string>& macros_;
		const std::unordered_map<std::string, std::string>& functionMacros_;
		std::unordered_set<std::string> expanding_;
		size_t position_ = 0;
		size_t recursionDepth_ = 0;
		bool reliable_ = true;
	};

	std::string maskInactivePreprocessorBranches(const std::string& source) {
		struct ConditionalFrame {
			bool parentActive = true;
			bool branchTaken = false;
			bool indeterminate = false;
		};
		struct Line {
			size_t start = 0;
			size_t end = 0;
			size_t contentEnd = 0;
		};

		std::vector<Line> lines;
		for (size_t start = 0; start < source.size();) {
			size_t end = source.find('\n', start);
			if (end == std::string::npos) end = source.size();
			else ++end;
			size_t contentEnd = end;
			while (contentEnd > start
				&& (source[contentEnd - 1] == '\n'
					|| source[contentEnd - 1] == '\r')) {
				--contentEnd;
			}
			lines.push_back({ start, end, contentEnd });
			start = end;
		}
		if (lines.empty()) return source;

		auto macros = g_state.toolchain.predefinedMacros;
		auto functionMacros = g_state.toolchain.predefinedFunctionMacros;
		std::vector<ConditionalFrame> stack;
		bool active = true;
		std::string result = source;

		auto firstIdentifier = [](const std::string& value) {
			size_t start = 0;
			while (start < value.size()
				&& std::isspace(static_cast<unsigned char>(value[start]))) {
				++start;
			}
			size_t end = start;
			while (end < value.size() && isIdentifierChar(value[end])) ++end;
			return value.substr(start, end - start);
		};

		for (size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
			const Line& line = lines[lineIndex];
			size_t first = line.start;
			while (first < line.contentEnd
				&& (source[first] == ' ' || source[first] == '\t')) {
				++first;
			}
			if (first < line.contentEnd && source[first] == '#') {
				std::string logical = source.substr(
					first + 1, line.contentEnd - first - 1);
				size_t lastLine = lineIndex;
				while (lastLine + 1 < lines.size()) {
					size_t tail = lines[lastLine].contentEnd;
					if (tail <= lines[lastLine].start
						|| source[tail - 1] != '\\') {
						break;
					}
					if (!logical.empty()) logical.pop_back();
					++lastLine;
					logical += source.substr(lines[lastLine].start,
						lines[lastLine].contentEnd - lines[lastLine].start);
				}
				if (!active) {
					for (size_t maskedLine = lineIndex;
						maskedLine <= lastLine; ++maskedLine) {
						for (size_t i = lines[maskedLine].start;
							i < lines[maskedLine].contentEnd; ++i) {
							result[i] = ' ';
						}
					}
				}

				size_t directiveStart = 0;
				while (directiveStart < logical.size()
					&& std::isspace(static_cast<unsigned char>(
						logical[directiveStart]))) {
					++directiveStart;
				}
				size_t directiveEnd = directiveStart;
				while (directiveEnd < logical.size()
					&& isIdentifierChar(logical[directiveEnd])) {
					++directiveEnd;
				}
				const std::string directive = logical.substr(
					directiveStart, directiveEnd - directiveStart);
				const std::string argument =
					trimCopy(logical.substr(directiveEnd));

				if (directive == "if" || directive == "ifdef"
					|| directive == "ifndef") {
					bool condition = false;
					bool reliable = true;
					if (directive == "ifdef") {
						const std::string name = firstIdentifier(argument);
						condition = macros.find(name) != macros.end()
							|| functionMacros.find(name)
								!= functionMacros.end();
					}
					else if (directive == "ifndef") {
						const std::string name = firstIdentifier(argument);
						condition = macros.find(name) == macros.end()
							&& functionMacros.find(name)
								== functionMacros.end();
					}
					else {
						PreprocessorExpression expression(
							argument, macros, functionMacros);
						condition = expression.evaluate() != 0;
						reliable = expression.reliable();
					}
					stack.push_back({
						active,
						active && reliable && condition,
						!reliable
					});
					active = reliable ? active && condition : active;
				}
				else if (directive == "elif" && !stack.empty()) {
					auto& frame = stack.back();
					if (frame.indeterminate) {
						active = frame.parentActive;
						lineIndex = lastLine;
						continue;
					}
					PreprocessorExpression expression(
						argument, macros, functionMacros);
					const bool condition = expression.evaluate() != 0;
					if (!expression.reliable()) {
						frame.indeterminate = true;
						active = frame.parentActive;
						lineIndex = lastLine;
						continue;
					}
					active = frame.parentActive
						&& !frame.branchTaken && condition;
					if (active) frame.branchTaken = true;
				}
				else if (directive == "else" && !stack.empty()) {
					auto& frame = stack.back();
					active = frame.indeterminate
						? frame.parentActive
						: frame.parentActive && !frame.branchTaken;
					frame.branchTaken = true;
				}
				else if (directive == "endif" && !stack.empty()) {
					active = stack.back().parentActive;
					stack.pop_back();
				}
				else if (directive == "define" && active) {
					const std::string name = firstIdentifier(argument);
					if (!name.empty()) {
						size_t valueStart = argument.find(name) + name.size();
						if (valueStart < argument.size()
							&& argument[valueStart] == '(') {
							functionMacros[name] =
								trimCopy(argument.substr(valueStart));
							macros.erase(name);
						}
						else {
							const std::string value =
								trimCopy(argument.substr(valueStart));
							macros[name] = value.empty() ? "1" : value;
							functionMacros.erase(name);
						}
					}
				}
				else if (directive == "undef" && active) {
					const std::string name = firstIdentifier(argument);
					macros.erase(name);
					functionMacros.erase(name);
				}
				lineIndex = lastLine;
				continue;
			}
			if (!active) {
				for (size_t i = line.start; i < line.contentEnd; ++i) {
					result[i] = ' ';
				}
			}
		}
		return result;
	}

	// コメント、文字列、プリプロセッサ行を空白にし、改行と位置だけを保つ。
	// これにより文字列中の '{' やマクロをトップレベル関数と誤認しない。
	std::string maskNonCode(const std::string& source) {
		enum class State { code, lineComment, blockComment, stringLiteral, charLiteral };
		State state = State::code;
		std::string masked(source.size(), ' ');
		bool lineHasCode = false;
		bool preprocessorLine = false;

		for (size_t i = 0; i < source.size(); ++i) {
			char c = source[i];
			char next = i + 1 < source.size() ? source[i + 1] : '\0';

			if (c == '\n') {
				size_t previous = i;
				while (previous > 0 && source[previous - 1] == '\r') {
					--previous;
				}
				const bool continuedPreprocessorLine =
					preprocessorLine && previous > 0
					&& source[previous - 1] == '\\';
				masked[i] = '\n';
				if (state == State::lineComment) state = State::code;
				lineHasCode = false;
				preprocessorLine = continuedPreprocessorLine;
				continue;
			}
			if (preprocessorLine) {
				continue;
			}
			if (state == State::lineComment) continue;
			if (state == State::blockComment) {
				if (c == '*' && next == '/') {
					state = State::code;
					++i;
				}
				continue;
			}
			if (state == State::stringLiteral || state == State::charLiteral) {
				if (c == '\\' && next != '\0') {
					++i;
					continue;
				}
				if ((state == State::stringLiteral && c == '"')
					|| (state == State::charLiteral && c == '\'')) {
					state = State::code;
				}
				continue;
			}
			if (!lineHasCode && (c == ' ' || c == '\t' || c == '\r')) {
				continue;
			}
			if (!lineHasCode && c == '#') {
				preprocessorLine = true;
				continue;
			}
			lineHasCode = true;
			if (c == '/' && next == '/') {
				state = State::lineComment;
				++i;
				continue;
			}
			if (c == '/' && next == '*') {
				state = State::blockComment;
				++i;
				continue;
			}
			if (c == 'R' && next == '"') {
				// C++ raw string: R"delimiter(contents)delimiter"。
				// u8R/LR等の接頭辞でも、Rの位置から残りを安全に隠せる。
				const size_t delimiterStart = i + 2;
				const size_t open = source.find('(', delimiterStart);
				if (open != std::string::npos
					&& open - delimiterStart <= 16) {
					bool validDelimiter = true;
					for (size_t j = delimiterStart; j < open; ++j) {
						const unsigned char d =
							static_cast<unsigned char>(source[j]);
						if (std::isspace(d) || source[j] == '('
							|| source[j] == ')' || source[j] == '\\') {
							validDelimiter = false;
							break;
						}
					}
					if (validDelimiter) {
						const std::string terminator = ")" +
							source.substr(delimiterStart,
								open - delimiterStart) + "\"";
						const size_t close = source.find(terminator, open + 1);
						const size_t rawEnd = close == std::string::npos
							? source.size() : close + terminator.size();
						for (size_t j = i; j < rawEnd; ++j) {
							if (source[j] == '\n') masked[j] = '\n';
						}
						i = rawEnd == 0 ? 0 : rawEnd - 1;
						continue;
					}
				}
			}
			if (c == '"') {
				state = State::stringLiteral;
				continue;
			}
			if (c == '\'') {
				state = State::charLiteral;
				continue;
			}
			masked[i] = c;
		}
		return masked;
	}

	bool isIdentifierChar(char c) {
		return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
	}

	std::vector<std::pair<size_t, size_t>> findParameterDefaultRanges(
		const std::string& signature,
		size_t openParen, size_t closeParen) {
		std::vector<std::pair<size_t, size_t>> ranges;
		if (openParen >= closeParen || closeParen > signature.size()) {
			return ranges;
		}
		const std::string masked = maskNonCode(signature);
		int parentheses = 0;
		int brackets = 0;
		int braces = 0;
		int angles = 0;
		bool skippingDefault = false;
		size_t defaultStart = 0;
		for (size_t i = openParen + 1; i < closeParen; ++i) {
			const char c = masked[i];
			if (skippingDefault) {
				if (c == '(') ++parentheses;
				else if (c == ')' && parentheses > 0) --parentheses;
				else if (c == '[') ++brackets;
				else if (c == ']' && brackets > 0) --brackets;
				else if (c == '{') ++braces;
				else if (c == '}' && braces > 0) --braces;
				else if (c == '<') ++angles;
				else if (c == '>' && angles > 0) --angles;
				else if (c == ',' && parentheses == 0 && brackets == 0
					&& braces == 0 && angles == 0) {
					ranges.emplace_back(defaultStart, i);
					skippingDefault = false;
				}
				continue;
			}
			if (c == '(') ++parentheses;
			else if (c == ')' && parentheses > 0) --parentheses;
			else if (c == '[') ++brackets;
			else if (c == ']' && brackets > 0) --brackets;
			else if (c == '{') ++braces;
			else if (c == '}' && braces > 0) --braces;
			else if (c == '<') ++angles;
			else if (c == '>' && angles > 0) --angles;
			else if (c == '=' && parentheses == 0 && brackets == 0
				&& braces == 0 && angles == 0) {
				defaultStart = i;
				skippingDefault = true;
			}
		}
		if (skippingDefault) {
			ranges.emplace_back(defaultStart, closeParen);
		}
		return ranges;
	}

	std::vector<FunctionDefinition> findTopLevelFunctions(
		const std::string& source) {
		const std::string masked =
			maskNonCode(maskInactivePreprocessorBranches(source));
		std::vector<FunctionDefinition> functions;
		size_t boundary = 0;
		int braceDepth = 0;

		for (size_t i = 0; i < masked.size(); ++i) {
			char c = masked[i];
			if (c == '{') {
				if (braceDepth++ != 0) continue;

				size_t end = i;
				while (end > boundary
					&& std::isspace(static_cast<unsigned char>(masked[end - 1]))) {
					--end;
				}
				if (end == boundary) continue;
				size_t closeParen = masked.rfind(')', end - 1);
				if (closeParen == std::string::npos || closeParen < boundary) continue;

				int parenDepth = 1;
				size_t openParen = closeParen;
				while (openParen > boundary && parenDepth > 0) {
					--openParen;
					if (masked[openParen] == ')') ++parenDepth;
					else if (masked[openParen] == '(') --parenDepth;
				}
				if (parenDepth != 0) continue;

				size_t nameEnd = openParen;
				while (nameEnd > boundary
					&& std::isspace(static_cast<unsigned char>(masked[nameEnd - 1]))) {
					--nameEnd;
				}
				size_t nameStart = nameEnd;
				while (nameStart > boundary
					&& isIdentifierChar(masked[nameStart - 1])) {
					--nameStart;
				}
				std::string name;
				if (nameStart == nameEnd) {
					// 記号演算子 (operator+, operator[], operator() 等) は
					// '(' の直前が識別子ではない。operator キーワードまで戻す。
					const size_t operatorStart = masked.rfind(
						"operator", nameEnd == 0 ? 0 : nameEnd - 1);
					const bool validOperator =
						operatorStart != std::string::npos
						&& operatorStart >= boundary
						&& (operatorStart == boundary
							|| !isIdentifierChar(masked[operatorStart - 1]))
						&& operatorStart + 8 <= nameEnd;
					if (!validOperator) continue;
					nameStart = operatorStart;
					name = "operator";
				}
				else {
					name = masked.substr(nameStart, nameEnd - nameStart);
				}
				static const std::set<std::string> controlWords = {
					"if", "for", "while", "switch", "catch"
				};
				if (controlWords.find(name) != controlWords.end()) continue;
				if (nameStart >= 2 && masked[nameStart - 1] == ':'
					&& masked[nameStart - 2] == ':') {
					continue; // クラスメンバー定義やコンストラクター
				}

				size_t signatureStart = boundary;
				while (signatureStart < nameStart
					&& std::isspace(static_cast<unsigned char>(
						masked[signatureStart]))) {
					++signatureStart;
				}
				if (signatureStart >= nameStart) continue; // 戻り値型がない

				const std::string prefix =
					masked.substr(signatureStart, nameStart - signatureStart);
				if (prefix.find('=') != std::string::npos
					|| prefix.find(']') != std::string::npos) {
					continue; // ラムダや初期化式
				}

				std::string signature = trimCopy(
					source.substr(signatureStart, end - signatureStart));
				if (signature.empty()) continue;
				const size_t relativeOpen = openParen - signatureStart;
				const size_t relativeClose = closeParen - signatureStart;
				auto defaultRanges = findParameterDefaultRanges(
					signature, relativeOpen, relativeClose);
				for (auto& range : defaultRanges) {
					range.first += signatureStart;
					range.second += signatureStart;
				}
				functions.push_back({
					signatureStart,
					signature + ";",
					std::move(defaultRanges)
				});
			}
			else if (c == '}') {
				if (braceDepth > 0) --braceDepth;
				if (braceDepth == 0) boundary = i + 1;
			}
			else if (c == ';' && braceDepth == 0) {
				boundary = i + 1;
			}
		}

		// extern "C" { ... } / extern "C++" { ... } はリンケージ指定であり、
		// 名前空間やクラスではない。ブロック直下の関数もArduino IDEと同様に
		// プロトタイプ生成の対象へ含める。
		boundary = 0;
		braceDepth = 0;
		for (size_t i = 0; i < masked.size(); ++i) {
			const char c = masked[i];
			if (c == '{') {
				if (braceDepth == 0) {
					const std::string header =
						trimCopy(source.substr(boundary, i - boundary));
					std::string compactHeader;
					for (const unsigned char ch : header) {
						if (!std::isspace(ch)) {
							compactHeader.push_back(static_cast<char>(ch));
						}
					}
					std::string linkage;
					if (compactHeader == "extern\"C\"") {
						linkage = "extern \"C\" ";
					}
					else if (compactHeader == "extern\"C++\"") {
						linkage = "extern \"C++\" ";
					}
					if (!linkage.empty()) {
						int nestedDepth = 1;
						size_t close = i + 1;
						for (; close < masked.size() && nestedDepth > 0; ++close) {
							if (masked[close] == '{') ++nestedDepth;
							else if (masked[close] == '}') --nestedDepth;
						}
						if (nestedDepth == 0) {
							const size_t contentStart = i + 1;
							const size_t contentEnd = close - 1;
							auto nested = findTopLevelFunctions(source.substr(
								contentStart, contentEnd - contentStart));
							for (auto& function : nested) {
								function.signatureStart += contentStart;
								function.prototype = linkage + function.prototype;
								for (auto& range : function.defaultRanges) {
									range.first += contentStart;
									range.second += contentStart;
								}
								functions.push_back(std::move(function));
							}
							i = close - 1;
							boundary = close;
							continue;
						}
					}
				}
				++braceDepth;
			}
			else if (c == '}') {
				if (braceDepth > 0) --braceDepth;
				if (braceDepth == 0) boundary = i + 1;
			}
			else if (c == ';' && braceDepth == 0) {
				boundary = i + 1;
			}
		}
		std::sort(functions.begin(), functions.end(),
			[](const FunctionDefinition& left, const FunctionDefinition& right) {
				return left.signatureStart < right.signatureStart;
			});
		return functions;
	}

	std::vector<std::string> orderInoFiles(
		std::vector<std::string> inos, const std::string& sketchDir) {
		std::sort(inos.begin(), inos.end());
		std::string primary = Utils::getFileName(sketchDir) + ".ino";
		std::transform(primary.begin(), primary.end(), primary.begin(),
			[](unsigned char c) { return (char)std::tolower(c); });
		auto it = std::find_if(inos.begin(), inos.end(), [&](const std::string& file) {
			std::string name = Utils::getFileName(file);
			std::transform(name.begin(), name.end(), name.begin(),
				[](unsigned char c) { return (char)std::tolower(c); });
			return name == primary;
		});
		if (it != inos.end() && it != inos.begin()) {
			std::rotate(inos.begin(), it, std::next(it));
		}
		return inos;
	}

	using LocalIncludeMap = std::unordered_map<std::string, std::string>;

	std::string stripUtf8Bom(std::string content) {
		if (content.size() >= 3
			&& static_cast<unsigned char>(content[0]) == 0xEF
			&& static_cast<unsigned char>(content[1]) == 0xBB
			&& static_cast<unsigned char>(content[2]) == 0xBF) {
			content.erase(0, 3);
		}
		return content;
	}

	std::string normalizeIncludeName(std::string name) {
		std::replace(name.begin(), name.end(), '\\', '/');
		std::vector<std::string> segments;
		size_t start = 0;
		while (start <= name.size()) {
			size_t slash = name.find('/', start);
			if (slash == std::string::npos) slash = name.size();
			const std::string segment = name.substr(start, slash - start);
			if (segment.empty() || segment == ".") {
				// skip
			}
			else if (segment == ".." && !segments.empty()
				&& segments.back() != "..") {
				segments.pop_back();
			}
			else {
				segments.push_back(segment);
			}
			if (slash == name.size()) break;
			start = slash + 1;
		}
		std::string normalized;
		for (const auto& segment : segments) {
			if (!normalized.empty()) normalized += '/';
			normalized += segment;
		}
		return normalized;
	}

	std::string rewriteLocalIncludes(const std::string& content,
		const LocalIncludeMap& includeMap,
		const std::string& currentDirectory = "") {
		std::string output;
		output.reserve(content.size());
		for (size_t start = 0; start < content.size();) {
			size_t lineEnd = content.find('\n', start);
			const bool hasNewline = lineEnd != std::string::npos;
			if (!hasNewline) lineEnd = content.size();
			std::string line = content.substr(start, lineEnd - start);

			size_t cursor = 0;
			while (cursor < line.size()
				&& (line[cursor] == ' ' || line[cursor] == '\t')) {
				++cursor;
			}
			if (cursor < line.size() && line[cursor++] == '#') {
				while (cursor < line.size()
					&& (line[cursor] == ' ' || line[cursor] == '\t')) {
					++cursor;
				}
				static const std::string includeWord = "include";
				if (line.compare(cursor, includeWord.size(), includeWord) == 0
					&& (cursor + includeWord.size() == line.size()
						|| !isIdentifierChar(line[cursor
							+ includeWord.size()]))) {
					cursor += includeWord.size();
					while (cursor < line.size()
						&& (line[cursor] == ' ' || line[cursor] == '\t')) {
						++cursor;
					}
					if (cursor < line.size()
						&& (line[cursor] == '"' || line[cursor] == '<')) {
						const char closing =
							line[cursor] == '"' ? '"' : '>';
						const size_t nameStart = ++cursor;
						const size_t nameEnd = line.find(closing, nameStart);
						if (nameEnd != std::string::npos) {
							const std::string name = normalizeIncludeName(
								line.substr(nameStart, nameEnd - nameStart));
							const std::string relativeName =
								currentDirectory.empty()
								? name
								: normalizeIncludeName(
									currentDirectory + "/" + name);
							auto it = includeMap.find(relativeName);
							if (it == includeMap.end()) {
								it = includeMap.find(name);
							}
							if (it != includeMap.end()) {
								line.replace(nameStart,
									nameEnd - nameStart, it->second);
							}
						}
					}
				}
			}
			output += line;
			if (hasNewline) output += '\n';
			start = hasNewline ? lineEnd + 1 : content.size();
		}
		return output;
	}

	std::string safeStagedName(const std::string& originalPath) {
		std::string extension = Utils::pathToUtf8(
			Utils::pathFromUtf8(originalPath).extension());
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		static const std::set<std::string> safeExtensions = {
			".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx",
			".inc", ".ipp", ".tpp", ".s"
		};
		if (safeExtensions.find(extension) == safeExtensions.end()) {
			extension = ".inc";
		}
		else if (extension == ".s") {
			extension = ".S";
		}
		return "file_" + Utils::sha1Hex(originalPath).substr(0, 16)
			+ extension;
	}

	bool isTextSketchFile(const std::string& originalPath) {
		std::string extension = Utils::pathToUtf8(
			Utils::pathFromUtf8(originalPath).extension());
		std::transform(extension.begin(), extension.end(), extension.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		static const std::set<std::string> textExtensions = {
			".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx",
			".inc", ".ipp", ".tpp", ".ino", ".s"
		};
		return textExtensions.find(extension) != textExtensions.end();
	}

	std::vector<std::string> extractIncludeTargets(
		const std::string& content) {
		std::vector<std::string> targets;
		for (size_t start = 0; start < content.size();) {
			size_t lineEnd = content.find('\n', start);
			if (lineEnd == std::string::npos) lineEnd = content.size();
			size_t cursor = start;
			while (cursor < lineEnd
				&& (content[cursor] == ' ' || content[cursor] == '\t')) {
				++cursor;
			}
			if (cursor < lineEnd && content[cursor++] == '#') {
				while (cursor < lineEnd
					&& (content[cursor] == ' ' || content[cursor] == '\t')) {
					++cursor;
				}
				static const std::string includeWord = "include";
				if (content.compare(cursor, includeWord.size(),
					includeWord) == 0) {
					cursor += includeWord.size();
					while (cursor < lineEnd
						&& (content[cursor] == ' '
							|| content[cursor] == '\t')) {
						++cursor;
					}
					if (cursor < lineEnd
						&& (content[cursor] == '"'
							|| content[cursor] == '<')) {
						const char closing = content[cursor] == '"' ? '"' : '>';
						const size_t targetStart = ++cursor;
						const size_t targetEnd =
							content.find(closing, targetStart);
						if (targetEnd != std::string::npos
							&& targetEnd <= lineEnd) {
							targets.push_back(normalizeIncludeName(
								content.substr(targetStart,
									targetEnd - targetStart)));
						}
					}
				}
			}
			start = lineEnd < content.size() ? lineEnd + 1 : content.size();
		}
		return targets;
	}

	std::string extractPreprocessorDirectives(const std::string& content) {
		std::string directives;
		bool continuation = false;
		for (size_t start = 0; start < content.size();) {
			size_t lineEnd = content.find('\n', start);
			const bool hasNewline = lineEnd != std::string::npos;
			if (!hasNewline) lineEnd = content.size();
			size_t contentEnd = lineEnd;
			if (contentEnd > start && content[contentEnd - 1] == '\r') {
				--contentEnd;
			}
			size_t first = start;
			while (first < contentEnd
				&& (content[first] == ' ' || content[first] == '\t')) {
				++first;
			}
			const bool directive = continuation
				|| (first < contentEnd && content[first] == '#');
			if (directive) {
				directives.append(content, start, lineEnd - start);
				directives += '\n';
			}
			continuation = directive && contentEnd > start
				&& content[contentEnd - 1] == '\\';
			start = hasNewline ? lineEnd + 1 : content.size();
		}
		return directives;
	}

	std::set<std::string> selectBundledLibraries(
		const std::vector<std::string>& allFiles,
		const std::unordered_map<std::string, std::string>& relativeByOriginal,
		const LocalIncludeMap& includeMap) {
		static const std::unordered_map<std::string, std::string> headerToLibrary = {
			{"EEPROM.h", "EEPROM"},
			{"Ethernet.h", "Ethernet"},
			{"EthernetClient.h", "Ethernet"},
			{"EthernetServer.h", "Ethernet"},
			{"EthernetUdp.h", "Ethernet"},
			{"HID.h", "HID"},
			{"LiquidCrystal.h", "LiquidCrystal"},
			{"SD.h", "SD"},
			{"Servo.h", "Servo"},
			{"SoftwareSerial.h", "SoftwareSerial"},
			{"SPI.h", "SPI"},
			{"Stepper.h", "Stepper"},
			{"TFT.h", "TFT"},
			{"Wire.h", "Wire"}
		};
		std::set<std::string> selected;
		for (const auto& original : allFiles) {
			if (!isTextSketchFile(original)) continue;
			const std::string relative = relativeByOriginal.at(original);
			const size_t slash = relative.rfind('/');
			const std::string currentDirectory = slash == std::string::npos
				? "" : relative.substr(0, slash);
			const std::string activeContent =
				maskInactivePreprocessorBranches(
					stripUtf8Bom(Utils::readFileLimited(
						original, MAX_SINGLE_SKETCH_FILE_BYTES)));
			for (const auto& target : extractIncludeTargets(activeContent)) {
				const std::string localCandidate = currentDirectory.empty()
					? target
					: normalizeIncludeName(currentDirectory + "/" + target);
				if (includeMap.find(localCandidate) != includeMap.end()
					|| includeMap.find(target) != includeMap.end()) {
					continue;
				}
				const size_t targetSlash = target.rfind('/');
				const std::string header = targetSlash == std::string::npos
					? target : target.substr(targetSlash + 1);
				auto library = headerToLibrary.find(header);
				if (library != headerToLibrary.end()) {
					selected.insert(library->second);
				}
			}
		}
		// Arduinoの標準ライブラリ依存。利用者が依存先を直接include
		// していなくても、Arduino IDEと同様に必要な翻訳単位を加える。
		if (selected.find("Ethernet") != selected.end()
			|| selected.find("SD") != selected.end()
			|| selected.find("TFT") != selected.end()) {
			selected.insert("SPI");
		}
		return selected;
	}

	// 全 .ino を 1 つの .cpp に結合してファイルに書き出す
	// Arduino IDE と同様に、主スケッチを先頭へ置き、トップレベル関数の
	// プロトタイプを最初の関数定義直前へ自動生成する。
	std::string mergeInoFiles(const std::vector<std::string>& inos,
		const std::string& sketchDir,
		const std::string& buildDir,
		const LocalIncludeMap& includeMap,
		const std::string& analysisPreamble,
		long long inoMaxMtime) {
		(void)inoMaxMtime;
		std::string outPath = Utils::joinPath(buildDir, "sketch.ino.cpp");

		struct InoSource {
			std::string path;
			std::string content;
			size_t combinedStart = 0;
		};
		std::vector<InoSource> sources;
		std::string combined;
		for (const auto& ino : orderInoFiles(inos, sketchDir)) {
			InoSource item;
			item.path = ino;
			item.content = rewriteLocalIncludes(
				stripUtf8Bom(Utils::readFileLimited(
					ino, MAX_SINGLE_SKETCH_FILE_BYTES)), includeMap);
			item.combinedStart = combined.size();
			combined += item.content;
			combined += '\n';
			sources.push_back(std::move(item));
		}

		auto analyzedFunctions =
			findTopLevelFunctions(analysisPreamble + combined);
		std::vector<FunctionDefinition> functions;
		for (auto& function : analyzedFunctions) {
			if (function.signatureStart < analysisPreamble.size()) continue;
			function.signatureStart -= analysisPreamble.size();
			for (auto& range : function.defaultRanges) {
				if (range.first < analysisPreamble.size()
					|| range.second < analysisPreamble.size()) {
					continue;
				}
				range.first -= analysisPreamble.size();
				range.second -= analysisPreamble.size();
			}
			functions.push_back(std::move(function));
		}
		std::string processed = combined;
		for (const auto& function : functions) {
			for (const auto& range : function.defaultRanges) {
				for (size_t i = range.first; i < range.second
					&& i < processed.size(); ++i) {
					if (processed[i] != '\r' && processed[i] != '\n') {
						processed[i] = ' ';
					}
				}
			}
		}
		std::string prototypes;
		if (!functions.empty()) {
			prototypes = "\n// Auto-generated Arduino function prototypes\n";
			std::set<std::string> unique;
			for (const auto& function : functions) {
				if (unique.insert(function.prototype).second) {
					prototypes += function.prototype;
					prototypes += '\n';
				}
			}
		}

		std::string out;
		out.reserve(combined.size() + prototypes.size() + sources.size() * 64 + 64);
		out += "// Auto-generated by arduino-build-daemon\n";
		out += "#include <Arduino.h>\n";
		bool insertedPrototypes = prototypes.empty();
		const size_t insertion = functions.empty()
			? std::string::npos : functions.front().signatureStart;
		for (const auto& ino : sources) {
			const std::string processedContent =
				processed.substr(ino.combinedStart, ino.content.size());
			out += "#line 1 \"";
			out += Utils::getFileName(ino.path);
			out += "\"\n";
			const size_t sourceEnd = ino.combinedStart + ino.content.size();
			if (!insertedPrototypes && insertion >= ino.combinedStart
				&& insertion <= sourceEnd) {
				const size_t local = insertion - ino.combinedStart;
				out.append(processedContent, 0, local);
				out += prototypes;
				const size_t originalLine = 1 + static_cast<size_t>(
					std::count(ino.content.begin(), ino.content.begin() + local, '\n'));
				out += "#line " + std::to_string(originalLine) + " \"";
				out += Utils::getFileName(ino.path);
				out += "\"\n";
				out.append(processedContent, local, std::string::npos);
				insertedPrototypes = true;
			}
			else {
				out += processedContent;
			}
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
		const std::string& buildDir, bool* hasIno = nullptr,
		std::vector<std::string>* libraryIncludeDirs = nullptr) {
		std::vector<std::string> inos;
		std::vector<std::string> cpps;
		std::vector<std::string> cs;
		std::vector<std::string> assemblies;
		std::vector<std::string> allFiles;
		std::unordered_map<std::string, std::string> relativeByOriginal;
		uint64_t totalFileBytes = 0;
		uint64_t totalTextBytes = 0;

		const fs::path sketchPath = Utils::pathFromUtf8(sketchDir);
		auto collectEntry = [&](const fs::directory_entry& entry,
			bool allowIno) {
			std::error_code fileEc;
			if (!entry.is_regular_file(fileEc)) return;
			const uintmax_t rawSize = entry.file_size(fileEc);
			if (fileEc) {
				throw std::runtime_error("Cannot inspect sketch file: "
					+ Utils::pathToUtf8(entry.path()));
			}
			const uint64_t fileSize = static_cast<uint64_t>(rawSize);
			if (allFiles.size() >= MAX_SKETCH_FILE_COUNT) {
				throw std::runtime_error(
					"Sketch contains more than "
					+ std::to_string(MAX_SKETCH_FILE_COUNT)
					+ " files; move generated or unrelated files outside src");
			}
			if (fileSize > MAX_SINGLE_SKETCH_FILE_BYTES) {
				throw std::runtime_error("Sketch file exceeds 32 MiB limit: "
					+ Utils::pathToUtf8(entry.path()));
			}
			if (fileSize > MAX_TOTAL_SKETCH_FILE_BYTES - totalFileBytes) {
				throw std::runtime_error(
					"Sketch files exceed 128 MiB total limit");
			}
			std::string ext = Utils::pathToUtf8(entry.path().extension());
			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return (char)std::tolower(c); });
			const std::string entryPath = Utils::pathToUtf8(entry.path());
			totalFileBytes += fileSize;
			if (isTextSketchFile(entryPath)) {
				if (fileSize > MAX_TOTAL_SKETCH_TEXT_BYTES - totalTextBytes) {
					throw std::runtime_error(
						"Sketch source and header files exceed 64 MiB total limit");
				}
				totalTextBytes += fileSize;
			}
			allFiles.push_back(entryPath);
			std::error_code relativeEc;
			fs::path relative = fs::relative(
				entry.path(), sketchPath, relativeEc);
			if (relativeEc) relative = entry.path().lexically_relative(sketchPath);
			relativeByOriginal[entryPath] = normalizeIncludeName(
				Utils::pathToUtf8(relative));
			if (allowIno && ext == ".ino") inos.push_back(entryPath);
			else if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") {
				cpps.push_back(entryPath);
			}
			else if (ext == ".c")   cs.push_back(entryPath);
			else if (ext == ".s")   assemblies.push_back(entryPath);
		};

		// Arduino仕様どおり、ルート直下とsrc/配下を再帰的に収集する。
		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(sketchPath, ec)) {
			if (ec) break;
			collectEntry(entry, true);
		}
		const fs::path sourceTree = sketchPath / L"src";
		ec.clear();
		if (fs::is_directory(sourceTree, ec)) {
			for (const auto& entry : fs::recursive_directory_iterator(
				sourceTree, fs::directory_options::skip_permission_denied, ec)) {
				if (ec) break;
				collectEntry(entry, false);
			}
		}
		inos = orderInoFiles(std::move(inos), sketchDir);
		std::sort(cpps.begin(), cpps.end());
		std::sort(cs.begin(), cs.end());
		std::sort(assemblies.begin(), assemblies.end());
		std::sort(allFiles.begin(), allFiles.end());
		if (hasIno) *hasIno = !inos.empty();

		// AVR GCC 7 (Windows) はUnicodeファイル名を含む#includeを開けない。
		// スケッチとsrc/のファイルをASCII名でステージし、引用形式のローカル
		// includeだけを対応する安全名へ書き換える。元ファイルは変更しない。
		const std::string stagedDir =
			Utils::joinPath(buildDir, "staged");
		Utils::createDirectory(stagedDir);
		LocalIncludeMap includeMap;
		std::unordered_map<std::string, std::string> stagedByOriginal;
		std::unordered_map<std::string, size_t> baseNameCounts;
		for (const auto& original : allFiles) {
			++baseNameCounts[normalizeIncludeName(
				Utils::getFileName(original))];
		}
		for (const auto& original : allFiles) {
			const std::string safeName = safeStagedName(original);
			const std::string relative = relativeByOriginal.at(original);
			includeMap[relative] = safeName;
			const std::string baseName = normalizeIncludeName(
				Utils::getFileName(original));
			if (baseNameCounts[baseName] == 1) {
				includeMap[baseName] = safeName;
			}
			stagedByOriginal[original] =
				Utils::joinPath(stagedDir, safeName);
		}
		const auto selectedLibraries = selectBundledLibraries(
			allFiles, relativeByOriginal, includeMap);
		std::vector<std::string> selectedLibrarySourceDirs;
		for (const auto& library : selectedLibraries) {
			const std::string sourceDir = Utils::joinPath(
				Utils::joinPath(g_state.toolchain.librariesDir, library),
				"src");
			selectedLibrarySourceDirs.push_back(sourceDir);
			if (libraryIncludeDirs) libraryIncludeDirs->push_back(sourceDir);
		}
		for (const auto& original : allFiles) {
			std::string extension = Utils::pathToUtf8(
				Utils::pathFromUtf8(original).extension());
			std::transform(extension.begin(), extension.end(), extension.begin(),
				[](unsigned char c) {
					return static_cast<char>(std::tolower(c));
				});
			if (extension == ".ino") continue;
			std::string content = Utils::readFileLimited(
				original, MAX_SINGLE_SKETCH_FILE_BYTES);
			if (isTextSketchFile(original)) {
				const std::string relative = relativeByOriginal.at(original);
				const size_t slash = relative.rfind('/');
				const std::string currentDirectory = slash == std::string::npos
					? "" : relative.substr(0, slash);
				content = rewriteLocalIncludes(
					stripUtf8Bom(std::move(content)), includeMap,
					currentDirectory);
				content = "#line 1 \"" + relative
					+ "\"\n" + content;
			}
			Utils::writeFileIfChanged(stagedByOriginal[original], content);
		}
		// ファイル名変更・削除のたびに古いステージファイルを残すと、長時間
		// 使う課題ほどキャッシュが増え続ける。現在の入力に対応しないものだけ
		// を、スケッチ単位mutex保持中に回収する。
		{
			std::unordered_set<std::string> currentStagedFiles;
			currentStagedFiles.reserve(stagedByOriginal.size());
			for (const auto& pair : stagedByOriginal) {
				currentStagedFiles.insert(pair.second);
			}
			std::error_code cleanupEc;
			for (const auto& entry : fs::directory_iterator(
				Utils::pathFromUtf8(stagedDir), cleanupEc)) {
				if (cleanupEc) break;
				std::error_code fileEc;
				if (!entry.is_regular_file(fileEc)) continue;
				const std::string path = Utils::pathToUtf8(entry.path());
				if (currentStagedFiles.find(path) == currentStagedFiles.end()) {
					Utils::deleteFile(path);
				}
			}
		}
		std::string analysisPreamble;
		for (const auto& original : allFiles) {
			std::string extension = Utils::pathToUtf8(
				Utils::pathFromUtf8(original).extension());
			std::transform(extension.begin(), extension.end(),
				extension.begin(), [](unsigned char c) {
					return static_cast<char>(std::tolower(c));
				});
			if (extension == ".h" || extension == ".hh"
				|| extension == ".hpp" || extension == ".hxx"
				|| extension == ".inc" || extension == ".ipp"
				|| extension == ".tpp") {
				analysisPreamble += extractPreprocessorDirectives(
					stripUtf8Bom(Utils::readFileLimited(
						original, MAX_SINGLE_SKETCH_FILE_BYTES)));
			}
		}

		std::vector<FileEntry> entries;
		entries.reserve(inos.empty()
			? cpps.size() + cs.size() + assemblies.size()
			: 1 + cpps.size() + cs.size() + assemblies.size());

		// 1) .ino 群を 1 つの .ino.cpp に統合
		if (!inos.empty()) {
			InoStats st = statInos(inos);
			std::string merged = mergeInoFiles(
				inos, sketchDir, buildDir, includeMap,
				analysisPreamble, st.maxMtime);
			FileEntry e;
			e.srcPath = merged;
			e.objPath = merged + ".o";
			e.isCpp = true;
			e.mtime = st.maxMtime;
			e.size = st.totalSize;
			e.contentHash = requiredFileHash(merged);
			entries.push_back(std::move(e));
		}
		// 2) .cpp
		for (const auto& src : cpps) {
			FileEntry e;
			e.srcPath = stagedByOriginal.at(src);
			e.objPath = Utils::joinPath(buildDir, "cpp_"
				+ Utils::sha1Hex(src).substr(0, 16) + ".o");
			e.isCpp = true;
			uint64_t sz = 0;
			Utils::getFileMetadata(src, e.mtime, sz);
			e.size = static_cast<uintmax_t>(sz);
			e.contentHash = requiredFileHash(e.srcPath);
			entries.push_back(std::move(e));
		}
		// 3) .c
		for (const auto& src : cs) {
			FileEntry e;
			e.srcPath = stagedByOriginal.at(src);
			e.objPath = Utils::joinPath(buildDir, "c_"
				+ Utils::sha1Hex(src).substr(0, 16) + ".o");
			e.isCpp = false;
			uint64_t sz = 0;
			Utils::getFileMetadata(src, e.mtime, sz);
			e.size = static_cast<uintmax_t>(sz);
			e.contentHash = requiredFileHash(e.srcPath);
			entries.push_back(std::move(e));
		}
		// 4) .S / .s (プリプロセッサ付きAVRアセンブリ)
		for (const auto& src : assemblies) {
			FileEntry e;
			e.srcPath = stagedByOriginal.at(src);
			e.objPath = Utils::joinPath(buildDir, "asm_"
				+ Utils::sha1Hex(src).substr(0, 16) + ".o");
			e.isCpp = false;
			e.isAssembler = true;
			uint64_t sz = 0;
			Utils::getFileMetadata(src, e.mtime, sz);
			e.size = static_cast<uintmax_t>(sz);
			e.contentHash = requiredFileHash(e.srcPath);
			entries.push_back(std::move(e));
		}
		// 5) 使用された同梱Arduino AVR標準ライブラリ
		std::vector<std::string> librarySources;
		for (const auto& sourceDir : selectedLibrarySourceDirs) {
			std::error_code libraryEc;
			if (!fs::is_directory(Utils::pathFromUtf8(sourceDir), libraryEc)) {
				continue;
			}
			for (const auto& entry : fs::recursive_directory_iterator(
				Utils::pathFromUtf8(sourceDir),
				fs::directory_options::skip_permission_denied, libraryEc)) {
				if (libraryEc) break;
				std::error_code fileEc;
				if (!entry.is_regular_file(fileEc)) continue;
				std::string extension =
					Utils::pathToUtf8(entry.path().extension());
				std::transform(extension.begin(), extension.end(),
					extension.begin(), [](unsigned char c) {
						return static_cast<char>(std::tolower(c));
					});
				if (extension == ".cpp" || extension == ".cc"
					|| extension == ".cxx" || extension == ".c"
					|| extension == ".s") {
					librarySources.push_back(
						Utils::pathToUtf8(entry.path()));
				}
			}
		}
		std::sort(librarySources.begin(), librarySources.end());
		for (const auto& src : librarySources) {
			std::string extension = Utils::pathToUtf8(
				Utils::pathFromUtf8(src).extension());
			std::transform(extension.begin(), extension.end(),
				extension.begin(), [](unsigned char c) {
					return static_cast<char>(std::tolower(c));
				});
			FileEntry e;
			e.srcPath = src;
			e.objPath = Utils::joinPath(buildDir, "library_"
				+ Utils::sha1Hex(src).substr(0, 16) + ".o");
			e.isCpp = extension == ".cpp"
				|| extension == ".cc" || extension == ".cxx";
			e.isAssembler = extension == ".s";
			uint64_t size = 0;
			Utils::getFileMetadata(src, e.mtime, size);
			e.size = static_cast<uintmax_t>(size);
			e.contentHash = requiredFileHash(src);
			entries.push_back(std::move(e));
		}

		// 削除済みソースのオブジェクト・依存ファイルも回収する。固定名の
		// sketch.elf/hexや署名類には触れず、ハッシュ名プレフィックスだけを
		// 対象にすることで削除範囲をbuildDir内へ限定する。
		{
			std::unordered_set<std::string> currentGeneratedFiles;
			currentGeneratedFiles.reserve(entries.size() * 3);
			for (const auto& entry : entries) {
				currentGeneratedFiles.insert(entry.objPath);
				currentGeneratedFiles.insert(entry.objPath + ".d");
				currentGeneratedFiles.insert(entry.objPath + ".depsig");
			}
			std::error_code cleanupEc;
			for (const auto& entry : fs::directory_iterator(
				Utils::pathFromUtf8(buildDir), cleanupEc)) {
				if (cleanupEc) break;
				std::error_code fileEc;
				if (!entry.is_regular_file(fileEc)) continue;
				const std::string fileName =
					Utils::pathToUtf8(entry.path().filename());
				const bool generated =
					fileName.rfind("cpp_", 0) == 0
					|| fileName.rfind("c_", 0) == 0
					|| fileName.rfind("asm_", 0) == 0
					|| fileName.rfind("library_", 0) == 0;
				const std::string path = Utils::pathToUtf8(entry.path());
				if (generated
					&& currentGeneratedFiles.find(path)
						== currentGeneratedFiles.end()) {
					Utils::deleteFile(path);
				}
			}
		}
		return entries;
	}

	// =============================================================================

	// .stamps ファイル: ファイル単位の内容署名を永続化
	// フォーマット: 各行 "<mtime> <size> <sha1> <srcPath>"

	// =============================================================================

	struct Stamp {
		long long mtime = 0;
		uintmax_t size = 0;
		std::string contentHash;
	};
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
			if (iss >> s.mtime >> s.size >> s.contentHash) {
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
			oss << e.mtime << " " << e.size << " " << e.contentHash
				<< " " << e.srcPath << "\n";
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
		std::string content = Utils::normalizeToUtf8(
			Utils::readFileLimited(depPath, 8 * 1024 * 1024));
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

	bool dependencySignature(const FileEntry& entry, std::string& signature) {
		const std::string depPath = entry.objPath + ".d";
		if (!Utils::fileExists(depPath) || !Utils::fileExists(entry.objPath)) return false;
		std::vector<std::string> dependencies;
		try {
			dependencies = readDepFile(depPath);
		}
		catch (...) {
			return false;
		}
		if (dependencies.empty()) return false;

		std::ostringstream material;
		try {
			// 依存元だけでなくオブジェクト自身も検証し、ディスク破損や外部からの
			// 書き換えを「変更なし」と誤認しない。
			const std::string objectHash =
				Utils::sha1FileHex(entry.objPath);
			if (objectHash.empty()) return false;
			material << "@object" << '\0' << objectHash << '\n';
			for (const auto& dependency : dependencies) {
				if (dependency.empty()) continue;
				if (!Utils::fileExists(dependency)) return false;
				const std::string dependencyHash =
					Utils::sha1FileHex(dependency);
				if (dependencyHash.empty()) return false;
				material << dependency << '\0' << dependencyHash << '\n';
			}
		}
		catch (...) {
			return false;
		}
		signature = Utils::sha1Hex(material.str());
		return !signature.empty();
	}

	bool dependenciesUnchanged(const FileEntry& entry) {
		const std::string signaturePath = entry.objPath + ".depsig";
		if (!Utils::fileExists(signaturePath)) return false;
		std::string current;
		if (!dependencySignature(entry, current)) return false;
		return trimmedFileEquals(signaturePath, current);
	}

	void updateDependencySignature(const FileEntry& entry) {
		const std::string signaturePath = entry.objPath + ".depsig";
		std::string current;
		if (dependencySignature(entry, current)) {
			Utils::writeFile(signaturePath, current + "\n");
		}
		else {
			// 依存を証明できないオブジェクトは次回必ず再コンパイルする。
			Utils::deleteFile(signaturePath);
		}
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
				|| it->second.contentHash.empty()
				|| it->second.contentHash != e.contentHash
				|| !Utils::fileExists(e.objPath);
			// ----- ヘッダ依存判定 (.d ファイル経由) -----
			if (!dirty) {
				dirty = !dependenciesUnchanged(e);
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
		std::string assembly;
	};

	CompileFlagsCache buildFlagsCache(const BoardConfig& bc,
		const std::string& sketchDir, const std::string& buildDir,
		const std::vector<std::string>& libraryIncludeDirs) {
		CompileFlagsCache c;
		c.cpp = cppFlags(
			bc, sketchDir, buildDir, libraryIncludeDirs);
		c.cc = cFlags(
			bc, sketchDir, buildDir, libraryIncludeDirs);
		c.assembly = assemblerFlags(
			bc, sketchDir, buildDir, libraryIncludeDirs);
		return c;
	}

	std::string compilerOutputPath(const std::string& path) {
		const std::string parent = Utils::getParentDirectory(path);
		return Utils::joinPath(
			Utils::getShortPath(parent), Utils::getFileName(path));
	}

	bool compileOne(const FileEntry& e, const CompileFlagsCache& flags,
		std::string& outputLog) {
		// 依存ヘッダ情報を <obj>.d に出力させる (差分判定でヘッダ更新を検出するため)
		//   -MMD: ユーザヘッダのみを依存として出力
		//   -MF : 出力先を明示 (buildDir に入らないのを防ぐ)
		const std::string& flagStr = e.isAssembler
			? flags.assembly : (e.isCpp ? flags.cpp : flags.cc);
		const std::string depPath = e.objPath + ".d";
		const std::string compilerDepPath = compilerOutputPath(depPath);
		const std::string compilerSourcePath = Utils::getShortPath(e.srcPath);
		const std::string compilerObjectPath = compilerOutputPath(e.objPath);
		// 結合は += で連鎖。中間 ostringstream を避ける。
		std::string args;
		args.reserve(flagStr.size() + compilerSourcePath.size()
			+ compilerObjectPath.size() + compilerDepPath.size() + 64);
		args += flagStr;
		args += " -MMD -MF \"";
		args += compilerDepPath;
		args += "\" \"";
		args += compilerSourcePath;
		args += "\" -o \"";
		args += compilerObjectPath;
		args += "\"";
		const std::string& compiler = e.isCpp
			? g_state.toolchain.avrGpp : g_state.toolchain.avrGcc;
		auto pr = runProcess(compiler, args, "", true);
		outputLog += colorizeOutput(pr.output);
		outputLog += colorizeOutput(pr.error);
		if (pr.exitCode != 0) {
			return false;
		}
		updateDependencySignature(e);
		return true;
	}
	bool runLink(const std::vector<FileEntry>& all, const BoardConfig& bc,
		const std::string& coreA, const std::string& elfPath, std::string& outputLog) {
		const std::string buildDir = Utils::getParentDirectory(elfPath);
		const std::string responseName = ".link-objects.rsp";
		const std::string responsePath = Utils::joinPath(buildDir, responseName);
		std::ostringstream response;
		for (const auto& e : all) {
			// 応答ファイルを読む古いAVR GCCはUTF-8パスを正しく扱えない。
			// 全オブジェクトはbuildDir直下のASCIIハッシュ名なので、作業
			// ディレクトリからの相対名だけを書けばUnicodeを完全に避けられる。
			response << '"' << Utils::getFileName(e.objPath) << "\"\n";
		}
		Utils::writeFile(responsePath, response.str());
		std::ostringstream args;
		args << "-w -Os -flto -fuse-linker-plugin -Wl,--gc-sections"
			<< " -mmcu=" << bc.mcu
			<< " -o \"" << Utils::getFileName(elfPath) << "\"";
		args << " @\"" << responseName << "\"";
		args << " \"" << Utils::getShortPath(coreA) << "\" -lm";
		auto pr = runProcess(
			g_state.toolchain.avrGcc, args.str(), buildDir, true);
		outputLog += pr.output;
		if (pr.exitCode != 0) { outputLog += pr.error; return false; }
		return true;
	}
	bool runObjcopy(const std::string& elfPath, const std::string& hexPath,
		std::string& outputLog) {
		std::string args = "-O ihex -R .eeprom \""
			+ Utils::getShortPath(elfPath) + "\" \""
			+ compilerOutputPath(hexPath) + "\"";
		auto pr = runProcess(g_state.toolchain.avrObjcopy, args, "", true);
		outputLog += pr.output;
		if (pr.exitCode != 0) { outputLog += pr.error; return false; }
		return true;
	}

	std::string artifactSignature(const std::string& elfPath,
		const std::string& hexPath) {
		if (!Utils::fileExists(elfPath) || !Utils::fileExists(hexPath)) return "";
		try {
			const std::string elfHash = Utils::sha1FileHex(elfPath);
			const std::string hexHash = Utils::sha1FileHex(hexPath);
			if (elfHash.empty() || hexHash.empty()) return "";
			std::ostringstream material;
			material << "elf\0" << elfHash << "\nhex\0" << hexHash;
			return Utils::sha1Hex(material.str());
		}
		catch (...) {
			return "";
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
		std::string buildDir = computeBuildDir(sketchDir);
		Utils::createDirectory(buildDir);
		// buildDir内の生成物は複数ファイルで1つのトランザクションを構成する。
		// 同じスケッチを別ウィンドウから同時実行しても相互に壊さない。
		NamedMutexGuard buildLock("build:" + buildDir);
		if (!buildLock.acquired()) {
			out.errorMessage = "Cannot lock build cache: " + buildLock.error();
			return out;
		}
		BoardConfig bc = resolveBoardConfig(req.fqbn);
		// ソース列挙と.ino存在確認を1回のディレクトリ走査で済ませる。
		bool hasIno = false;
		std::vector<std::string> libraryIncludeDirs;
		auto entries = collectSources(
			sketchDir, buildDir, &hasIno, &libraryIncludeDirs);
		if (!hasIno) {
			out.errorMessage = "No .ino file found in: " + sketchDir;
			return out;
		}
		// --- core.a の確保 (初回のみ) ---
		std::string coreA = coreArchivePath(coreCacheKey(bc));
		{
			std::string err;
			if (!ensureCoreArchive(bc, coreA, err)) {
				out.errorMessage = err;
				return out;
			}
		}
		// --- 差分判定 ---
		// コンパイラフラグやツールチェーンが変わった場合は、mtimeが同じでも
		// 旧オブジェクトを再利用しない。通常時は短い署名ファイルを読むだけ。
		CompileFlagsCache flagsCache = buildFlagsCache(
			bc, sketchDir, buildDir, libraryIncludeDirs);
		std::string buildSignature = Utils::sha1Hex(
			flagsCache.cpp + "\n" + flagsCache.cc + "\n" +
			flagsCache.assembly + "\n" +
			g_state.toolchain.compilerVersion + "\n" + coreA);
		std::string signaturePath = Utils::joinPath(buildDir, ".build-signature");
		std::string invalidMarker = Utils::joinPath(buildDir, ".build-invalid");
		bool buildConfigChanged = !Utils::fileExists(signaturePath)
			|| !trimmedFileEquals(signaturePath, buildSignature);
		auto saved = loadStamps(buildDir);
		auto plan = planRebuild(entries, saved,
			req.forceFullBuild || buildConfigChanged);
		std::string elfPath = Utils::joinPath(buildDir, "sketch.elf");
		std::string hexPath = Utils::joinPath(buildDir, "sketch.hex");
		std::string artifactSignaturePath =
			Utils::joinPath(buildDir, ".artifact-signature");
		out.totalFiles = (int)entries.size();
		out.recompiledFiles = (int)plan.size();
		const bool buildWasInterrupted = Utils::fileExists(invalidMarker);
		const std::string currentArtifactSignature =
			artifactSignature(elfPath, hexPath);
		const bool artifactsValid = !currentArtifactSignature.empty()
			&& Utils::fileExists(artifactSignaturePath)
			&& trimmedFileEquals(
				artifactSignaturePath, currentArtifactSignature);
		// sidecarと内容ハッシュが同時に改変されても、曖昧・範囲外・
		// ブートローダー領域を含むHEXをキャッシュ成功扱いしない。
		std::string cachedHexValidationError;
		std::vector<uint8_t> cachedFlash;
		const bool cachedHexValid = artifactsValid
			&& Stk500v2::readIntelHex(
				hexPath, cachedFlash, cachedHexValidationError);
		// --- 完全キャッシュヒット: .hex と .elf があり、変更ゼロ ---
		if (plan.empty() && !buildWasInterrupted
			&& cachedHexValid) {
			out.success = true;
			out.cached = true;
			out.hexFile = hexPath;
			out.elfFile = elfPath;
			out.buildTimeMs = totalSw.elapsedMilliseconds();
			// state にも反映 (upload で参照される)
			{
				std::lock_guard<std::mutex> lk(g_state.sketchMtx);
				auto& s = g_state.sketches[sketchDir];
				s.sketchDir = sketchDir;
				s.buildDir = buildDir;
				s.hexFile = hexPath;
				s.elfFile = elfPath;
				s.hasValidBuild = true;
			}
			maintainBuildCache(buildDir);
			return out;
		}
		// ここから先は古いHEXを成功結果として扱わない。途中でコンパイル、
		// リンク、プロセス自体が失敗しても、このマーカーが次回の再リンクを保証する。
		Utils::writeFile(invalidMarker, "incomplete\n");
		{
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto& state = g_state.sketches[sketchDir];
			state.hasValidBuild = false;
			state.flashImage.reset();
			state.flashHexMtime = 0;
			state.flashHexSize = 0;
			state.flashHexHash.clear();
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
			|| !artifactsValid
			|| !cachedHexValid
			|| buildWasInterrupted;
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
		std::vector<uint8_t> validatedFlash;
		std::string hexValidationError;
		if (!Stk500v2::readIntelHex(
			hexPath, validatedFlash, hexValidationError)) {
			out.errorMessage =
				"Generated HEX validation failed: " + hexValidationError;
			return out;
		}
		const std::string verifiedArtifactSignature =
			artifactSignature(elfPath, hexPath);
		if (verifiedArtifactSignature.empty()) {
			out.errorMessage = "Cannot verify build artifacts";
			return out;
		}
		// --- スタンプ更新 ---
		saveStamps(buildDir, entries);
		Utils::writeFileIfChanged(signaturePath, buildSignature + "\n");
		Utils::writeFileIfChanged(artifactSignaturePath,
			verifiedArtifactSignature + "\n");
		Utils::deleteFile(invalidMarker);
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
			s.flashImage.reset();
			s.flashHexMtime = 0;
			s.flashHexSize = 0;
			s.flashHexHash.clear();
		}
		out.success = true;
		out.cached = false;
		out.hexFile = hexPath;
		out.elfFile = elfPath;
		out.buildTimeMs = totalSw.elapsedMilliseconds();
		maintainBuildCache(buildDir);
		return out;
	}
	UploadResult upload(const UploadRequest& req) {
		UploadResult out;
		std::string sketchDir = resolveSketchDir(req.sketchDir);
		if (sketchDir.empty()) {
			out.errorMessage = "Cannot resolve sketch directory: \"" + req.sketchDir + "\"";
			return out;
		}
		// --- ポート決定とプロセス間排他 ---
		// 複数のVS Codeウィンドウや互換daemonが同じボードへ同時にDTRを
		// 操作しないよう、コンパイル開始前からポート単位で排他する。
		std::string port = req.port;
		bool portsDetected = false;
		if (port.empty()) {
			std::lock_guard<std::mutex> lk(g_state.portMtx);
			port = g_state.cachedArduinoPort;
			portsDetected = !g_state.cachedPorts.empty();
		}
		if (port.empty()) {
			out.errorMessage = !portsDetected
				? "No COM port detected"
				: "Arduino port is ambiguous; connect only the target board or specify its COM port";
			return out;
		}
		if (!isValidComPortName(port)) {
			out.errorMessage = "Invalid COM port name: " + port;
			return out;
		}
		port = normalizeComPortName(std::move(port));
		out.port = port;
		NamedMutexGuard portUploadLock("upload-port:" + port, 0);
		if (!portUploadLock.acquired()) {
			out.errorMessage = "Cannot acquire exclusive upload access for "
				+ port + ": " + portUploadLock.error();
			return out;
		}

		const std::string buildDir = computeBuildDir(sketchDir);
		// コンパイル直後からHEX検証・実機書込み完了まで同じキャッシュを
		// 保持する。別プロセスの再ビルドや保守削除とのTOCTOUを防ぐ。
		NamedMutexGuard uploadBuildLock("build:" + buildDir);
		if (!uploadBuildLock.acquired()) {
			out.errorMessage =
				"Cannot lock upload build cache: " + uploadBuildLock.error();
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
		// =====================================================================

		// ネイティブ STK500v2 アップロード (avrdude を使わない)

		// =====================================================================

		std::shared_ptr<const std::vector<uint8_t>> flash;
		long long hexMtime = 0;
		uint64_t hexSize = 0;
		if (!Utils::getFileMetadata(
			out.compile.hexFile, hexMtime, hexSize)) {
			out.errorMessage = "Compiled HEX file is missing";
			return out;
		}
		if (hexSize > MAX_COMPILED_HEX_BYTES) {
			out.errorMessage =
				"Compiled HEX exceeds 4 MiB safety limit";
			return out;
		}
		std::string hexHash;
		try {
			hexHash = Utils::sha1FileHex(out.compile.hexFile);
			if (hexHash.empty()) {
				throw std::runtime_error("HEX hashing failed");
			}
		}
		catch (const std::exception& e) {
			out.errorMessage = "Cannot verify HEX before upload: "
				+ std::string(e.what());
			return out;
		}
		bool flashCacheHit = false;
		{
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto it = g_state.sketches.find(sketchDir);
			if (it != g_state.sketches.end()
				&& it->second.flashImage
				&& it->second.flashHexMtime == hexMtime
				&& it->second.flashHexSize == hexSize
				&& it->second.flashHexHash == hexHash) {
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
			try {
				if (Utils::sha1FileHex(out.compile.hexFile) != hexHash) {
					out.errorMessage =
						"HEX changed while it was being prepared for upload";
					return out;
				}
			}
			catch (const std::exception& e) {
				out.errorMessage = "Cannot recheck HEX before upload: "
					+ std::string(e.what());
				return out;
			}
			flash = parsed;
			std::lock_guard<std::mutex> lk(g_state.sketchMtx);
			auto& state = g_state.sketches[sketchDir];
			state.flashImage = parsed;
			state.flashHexMtime = hexMtime;
			state.flashHexSize = hexSize;
			state.flashHexHash = hexHash;
		}
		// 注: 旧 warmupComPort は削除。SerialPort::open() が直後に同じ COM を開くため重複していた。
		auto uploadStarted = std::chrono::steady_clock::now();
		auto stats = Stk500v2::uploadMega2560(port, *flash);
		int attempts = 1;
		std::string firstAttemptError;
		if (!stats.success && stats.retryable) {
			// 瞬間的なUSB通信エラーや検証不一致は、ポートを開き直して最初から1回だけ再試行する。
			firstAttemptError = stats.errorMessage;
			Sleep(50);
			stats = Stk500v2::uploadMega2560(port, *flash);
			attempts = 2;
		}
		// デバッグ用に内訳もログに残す
		std::ostringstream oss;
		oss << "stk500v2 native upload\n"
			<< "  attempts=" << attempts << "\n"
			<< "  open=" << stats.openMs << "ms"
			<< " reset=" << stats.resetMs << "ms"
			<< " sync=" << stats.syncMs << "ms"
			<< " prog=" << stats.progMs << "ms"
			<< " verify=" << stats.verifyMs << "ms"
			<< " leave=" << stats.leaveMs << "ms\n"
			<< "  pages=" << stats.pagesWritten
			<< " bytes=" << stats.bytesWritten
			<< " verified_pages=" << stats.pagesVerified
			<< " verified_bytes=" << stats.bytesVerified
			<< " flash_cache=" << (flashCacheHit ? "hit" : "miss")
			<< "\n";
		if (!firstAttemptError.empty()) {
			oss << "  first_attempt_error=" << firstAttemptError << "\n";
		}
		out.avrdudeOutput = oss.str();
		if (!stats.success) {
			out.errorMessage = "Upload failed after 2 attempts: " + stats.errorMessage;
			return out;
		}
		out.success = true;
		out.uploadTimeMs = std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - uploadStarted).count();
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
