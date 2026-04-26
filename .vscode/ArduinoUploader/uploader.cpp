#include "uploader.h"
#include "utils.h"

#include <windows.h>
#include <comdef.h>
#include <Wbemidl.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

// -----------------------------------------------------------------------------
// COM 補助: MSVC は comutil.h の _bstr_t、MinGW は自前ラッパで代用
// -----------------------------------------------------------------------------
#if defined(_MSC_VER)
#include <comutil.h>
using ComBStr = _bstr_t;
#else
// MinGW: comutil/comsuppw が使えないので自前で BSTR ラッパを定義
#include <initguid.h>
DEFINE_GUID(CLSID_WbemLocator, 0x4590f011, 0x52d3, 0x11d1,
	0xa7, 0x7f, 0x00, 0xc0, 0x4f, 0xd6, 0x08, 0xe4);
DEFINE_GUID(IID_IWbemLocator, 0xdc12a687, 0x737f, 0x11cf,
	0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24);

namespace {
	class BStrWrapper {
	public:
		BSTR bstr = nullptr;
		explicit BStrWrapper(const wchar_t* str) { bstr = SysAllocString(str); }
		explicit BStrWrapper(const char* str) {
			int len = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
			std::wstring wstr(len, L'\0');
			MultiByteToWideChar(CP_ACP, 0, str, -1, &wstr[0], len);
			bstr = SysAllocString(wstr.c_str());
		}
		~BStrWrapper() { if (bstr) SysFreeString(bstr); }
		BStrWrapper(const BStrWrapper&) = delete;
		BStrWrapper& operator=(const BStrWrapper&) = delete;
		operator BSTR() const { return bstr; }
	};
} // namespace
using ComBStr = BStrWrapper;
#endif

// =============================================================================
// 定数 (ボード固有設定)
// =============================================================================
namespace {

	// ターゲットボード: Arduino Mega ADK (ATmega2560)
	constexpr const char* kFqbn = "arduino:avr:megaADK";
	constexpr const char* kMcu = "atmega2560";
	constexpr const char* kAvrdudeMcu = "m2560";
	constexpr const char* kAvrdudeProg = "wiring";
	constexpr const char* kAvrdudeBaud = "115200";
	constexpr const char* kVariantSubdir = "variants\\mega";

	// Arduino IDE/CLI の標準パス
	constexpr const char* kAvrGccBase = "Arduino15\\packages\\arduino\\tools\\avr-gcc";
	constexpr const char* kAvrdudeBase = "Arduino15\\packages\\arduino\\tools\\avrdude";
	constexpr const char* kHwAvrBase = "Arduino15\\packages\\arduino\\hardware\\avr";

	// ソースファイルとして扱う拡張子(タイムスタンプ監視用)
	const std::vector<std::string> kWatchedExtensions = { ".h", ".hpp", ".cpp", ".c" };

	// シリアルデバイスの判別パターン
	const std::vector<std::string> kSerialDevicePatterns = {
		"Arduino", "Mega", "USB Serial", "CH340", "CP210", "FTDI"
	};

	// COM ポート照会の WMI クエリ
	// Win32_PnPEntity の Name には "Arduino Mega 2560 (COM3)" のような形式で
	// ポート番号が含まれるデバイスがある。WHERE は緩めにして、判定は C++ 側で行う。
	constexpr const wchar_t* kWmiQuery =
		L"SELECT Name FROM Win32_PnPEntity WHERE Name LIKE '%(COM%'";

	// 前方宣言: uploadToBoard() より前に必要(実装はファイル下方の COM 用 namespace 内)
	bool comPortIsOpenable(const std::string& port);

} // namespace

// =============================================================================
// Impl: 内部状態
// =============================================================================
struct Uploader::Impl {
	// 入力
	std::string sketchDir;
	std::string requestedPort;

	// グローバルキャッシュ(ツールチェーンパスなど)
	std::string globalCacheDir;
	std::string portCacheFile;
	std::string avrGccCacheFile;
	std::string hwCacheFile;
	std::string avrdudeCacheFile;

	// 解決済みのツールチェーン
	std::string avrGccRoot;
	std::string avrGpp;
	std::string avrGcc;
	std::string avrObjcopy;
	std::string hwRoot;
	std::string coresInc;
	std::string variantsInc;
	std::string avrdude;
	std::string avrdudeConf;

	// ターゲット
	std::string targetPort;

	// スケッチ固有のビルドパス
	std::string sketchName;
	std::string inoFile;
	std::string buildPath;
	std::string sketchBuild;
	std::string cppFile;
	std::string objFile;
	std::string elfFile;
	std::string hexFile;
	std::string coreA;
	std::string hashCacheFile;

	// ビルド状態
	std::string currentSignature;
	bool needCompile = true;
	bool isFullBuild = false;
	double buildTime = 0.0;
	double uploadTime = 0.0;
	std::string lastError;

	Impl(const std::string& sketchDir_, const std::string& port_)
		: sketchDir(sketchDir_), requestedPort(port_) {
		globalCacheDir = Utils::getGlobalCacheDir();
		portCacheFile = Utils::joinPath(globalCacheDir, "port_cache.txt");
		avrGccCacheFile = Utils::joinPath(globalCacheDir, "avr_gcc_path_cache.txt");
		hwCacheFile = Utils::joinPath(globalCacheDir, "hw_path_cache.txt");
		avrdudeCacheFile = Utils::joinPath(globalCacheDir, "avrdude_path_cache.txt");
	}
};

// =============================================================================
// Uploader: 公開 API
// =============================================================================
Uploader::Uploader(const std::string& sketchDir, const std::string& port)
	: pImpl(std::make_unique<Impl>(sketchDir, port)) {
}

Uploader::~Uploader() = default;

std::string Uploader::getLastError() const { return pImpl->lastError; }

bool Uploader::initialize() {
	try {
		Utils::createDirectory(pImpl->globalCacheDir);

		if (!resolveToolchainPaths()) return false;
		if (!resolvePort())           return false;
		if (!resolveBuildPaths())     return false;
		if (!checkCache())            return false;

		return true;
	} catch (const std::exception& e) {
		pImpl->lastError = std::string("Initialization error: ") + e.what();
		return false;
	}
}

bool Uploader::upload() {
	Stopwatch totalSw;

	std::cout << "Using Port: " << pImpl->targetPort << "\n\n";

	// avrdude のパス解決
	Stopwatch sw;
	if (!resolveAvrdudePaths()) return false;
	std::cout << "Avrdude path resolve: "
		<< std::fixed << std::setprecision(0)
		<< sw.elapsedMilliseconds() << "ms\n";

	// コンパイル
	sw.restart();
	if (!compile()) return false;
	pImpl->buildTime = sw.elapsedSeconds();

	if (pImpl->needCompile) {
		const char* tag = pImpl->isFullBuild ? "Compile (full): " : "Compile (incremental): ";
		std::cout << tag << std::fixed << std::setprecision(2) << pImpl->buildTime << "s\n";
	} else {
		std::cout << "Compile: skipped (no changes)\n";
	}

	// 書き込み
	sw.restart();
	if (!uploadToBoard()) return false;
	pImpl->uploadTime = sw.elapsedSeconds();
	std::cout << "Upload: " << std::fixed << std::setprecision(2)
		<< pImpl->uploadTime << "s\n";

	double totalTime = totalSw.elapsedSeconds();
	std::cout << "\nBuild: " << std::fixed << std::setprecision(2) << pImpl->buildTime
		<< "s  Upload: " << std::fixed << std::setprecision(2) << pImpl->uploadTime
		<< "s  Total: " << std::fixed << std::setprecision(2) << totalTime << "s\n";
	return true;
}

// =============================================================================
// 初期化フェーズ
// =============================================================================
bool Uploader::resolveToolchainPaths() {
	try {
		const std::string appData = Utils::getLocalAppDataPath();

		pImpl->avrGccRoot = resolveCachedDirectory(
			pImpl->avrGccCacheFile,
			Utils::joinPath(appData, kAvrGccBase),
			"avr-gcc not found",
			"\\bin");

		pImpl->avrGpp = Utils::joinPath(pImpl->avrGccRoot, "avr-g++.exe");
		pImpl->avrGcc = Utils::joinPath(pImpl->avrGccRoot, "avr-gcc.exe");
		pImpl->avrObjcopy = Utils::joinPath(pImpl->avrGccRoot, "avr-objcopy.exe");

		pImpl->hwRoot = resolveCachedDirectory(
			pImpl->hwCacheFile,
			Utils::joinPath(appData, kHwAvrBase),
			"arduino hardware not found");

		pImpl->coresInc = Utils::joinPath(pImpl->hwRoot, "cores\\arduino");
		pImpl->variantsInc = Utils::joinPath(pImpl->hwRoot, kVariantSubdir);

		return true;
	} catch (const std::exception& e) {
		pImpl->lastError = std::string("Path resolution error: ") + e.what();
		return false;
	}
}

bool Uploader::resolvePort() {
	try {
		// 1) 明示指定されたポート
		if (!pImpl->requestedPort.empty()) {
			pImpl->targetPort = pImpl->requestedPort;
			return true;
		}

		// 2) キャッシュ
		if (Utils::fileExists(pImpl->portCacheFile)) {
			try {
				std::string cached = Utils::trim(Utils::readFile(pImpl->portCacheFile));
				if (!cached.empty()) {
					pImpl->targetPort = cached;
					return true;
				}
			} catch (...) {
				// キャッシュ破損は無視して再検索
			}
		}

		// 3) WMI で検索
		pImpl->targetPort = findArduinoPort();
		if (pImpl->targetPort.empty()) {
			pImpl->lastError = "No COM port found";
			return false;
		}

		Utils::writeFileIfChanged(pImpl->portCacheFile, pImpl->targetPort);
		return true;
	} catch (const std::exception& e) {
		pImpl->lastError = std::string("Port resolution error: ") + e.what();
		return false;
	}
}

bool Uploader::resolveBuildPaths() {
	pImpl->sketchName = Utils::getFileName(pImpl->sketchDir);
	pImpl->inoFile = Utils::joinPath(pImpl->sketchDir, pImpl->sketchName + ".ino");

	if (!Utils::fileExists(pImpl->inoFile)) {
		pImpl->lastError = "Sketch file not found: " + pImpl->inoFile;
		return false;
	}

	// ビルド出力先: <cwd>/.vscode/build/<sketch>
	char cwd[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, cwd);
	std::string sketchCacheDir = Utils::joinPath(
		Utils::joinPath(Utils::joinPath(cwd, ".vscode"), "build"),
		pImpl->sketchName);
	Utils::createDirectory(sketchCacheDir);

	pImpl->buildPath = sketchCacheDir;
	pImpl->sketchBuild = Utils::joinPath(pImpl->buildPath, "sketch");
	pImpl->cppFile = Utils::joinPath(pImpl->sketchBuild, pImpl->sketchName + ".ino.cpp");
	pImpl->objFile = Utils::joinPath(pImpl->sketchBuild, pImpl->sketchName + ".ino.cpp.o");
	pImpl->elfFile = Utils::joinPath(pImpl->buildPath, pImpl->sketchName + ".ino.elf");
	pImpl->hexFile = Utils::joinPath(pImpl->buildPath, pImpl->sketchName + ".ino.hex");
	pImpl->coreA = Utils::joinPath(pImpl->buildPath, "core\\core.a");
	pImpl->hashCacheFile = Utils::joinPath(pImpl->buildPath, pImpl->sketchName + ".ino.hash");

	// 監視対象のソースを集めてシグネチャを計算
	std::vector<std::string> srcFiles = { pImpl->inoFile };
	for (const auto& ext : kWatchedExtensions) {
		auto files = Utils::getFilesByExtension(pImpl->sketchDir, ext);
		srcFiles.insert(srcFiles.end(), files.begin(), files.end());
	}
	pImpl->currentSignature = Utils::getSourceSignature(srcFiles);
	return true;
}

bool Uploader::checkCache() {
	pImpl->needCompile = true;
	if (!Utils::fileExists(pImpl->hashCacheFile) || !Utils::fileExists(pImpl->hexFile)) {
		return true;
	}
	try {
		std::string cached = Utils::trim(Utils::readFile(pImpl->hashCacheFile));
		if (cached == pImpl->currentSignature) {
			pImpl->needCompile = false;
		}
	} catch (...) {
		// ハッシュ読み込み失敗 -> 再ビルド
	}
	return true;
}

// =============================================================================
// ビルドフェーズ
// =============================================================================
bool Uploader::compile() {
	if (!pImpl->needCompile) return true;

	// core.a が無ければフルビルド、あればインクリメンタル
	if (!Utils::fileExists(pImpl->coreA)) {
		return fullCompile();
	}
	return incrementalCompile();
}

bool Uploader::fullCompile() {
	try {
		std::cout << "First build: generating core.a via arduino-cli...\n";

		std::ostringstream args;
		// --quiet: arduino-cli の出力を最小化(パイプ転送負荷を削減)
		// --no-color: ANSI エスケープを抑止
		args << "compile -b " << kFqbn
			<< " -j 0 --quiet --no-color"
			<< " --build-path \"" << pImpl->buildPath << "\""
			<< " --warnings none"
			<< " \"" << pImpl->sketchDir << "\"";

		// captureOutput=false: stdout/stderr をメモリに溜めない -> 高速化
		// (失敗時のみ詳細出力が必要なら、ここを true に切り替えるか
		//  --verbose を追加して捕捉する設計でも良い)
		auto result = runProcess("arduino-cli", args.str(), pImpl->buildPath, /*captureOutput=*/false);
		if (result.exitCode != 0) {
			// 失敗時は詳細出力を取り直すために再実行する選択肢もあるが、
			// ここではシンプルにエラーを返す
			pImpl->lastError = "Full build failed (arduino-cli exit code "
				+ std::to_string(result.exitCode) + ")";
			return false;
		}

		pImpl->isFullBuild = true;
		Utils::writeFile(pImpl->hashCacheFile, pImpl->currentSignature);
		return true;
	} catch (const std::exception& e) {
		pImpl->lastError = std::string("Full compile error: ") + e.what();
		return false;
	}
}

bool Uploader::incrementalCompile() {
	try {
		Utils::createDirectory(pImpl->sketchBuild);
		// 古い .h コピーが残っていれば削除(安全のため)
		Utils::cleanDirectory(pImpl->sketchBuild, ".h");

		// .ino を .cpp としてラップ
		// 内容が同じなら書き込まない(タイムスタンプ更新によるフルリビルド防止)
		std::string cppContent = "#include <Arduino.h>\r\n" + Utils::readFile(pImpl->inoFile);
		Utils::writeFileIfChanged(pImpl->cppFile, cppContent);

		// ---- avr-g++ でコンパイル ----
		std::ostringstream compileArgs;
		compileArgs << "-c -g -Os -w "
			<< "-std=gnu++11 -fpermissive -fno-exceptions "
			<< "-ffunction-sections -fdata-sections "
			<< "-fno-threadsafe-statics -Wno-error=narrowing "
			<< "-MMD -flto "
			<< "-mmcu=" << kMcu << " "
			<< "-DF_CPU=16000000L -DARDUINO=10607 "
			<< "-DARDUINO_AVR_ADK -DARDUINO_ARCH_AVR "
			<< "-I\"" << pImpl->coresInc << "\" "
			<< "-I\"" << pImpl->variantsInc << "\" "
			<< "-I\"" << pImpl->sketchDir << "\" "
			<< "\"" << pImpl->cppFile << "\" "
			<< "-o \"" << pImpl->objFile << "\"";
		auto cr = runProcess(pImpl->avrGpp, compileArgs.str(), pImpl->sketchBuild);
		if (cr.exitCode != 0) {
			pImpl->lastError = "Compile failed:\n" + cr.error;
			return false;
		}

		// ---- avr-gcc でリンク ----
		std::ostringstream linkArgs;
		linkArgs << "-w -Os -g -flto -fuse-linker-plugin -Wl,--gc-sections "
			<< "-mmcu=" << kMcu << " "
			<< "-o \"" << pImpl->elfFile << "\" "
			<< "\"" << pImpl->objFile << "\" "
			<< "\"" << pImpl->coreA << "\" "
			<< "-L\"" << pImpl->buildPath << "\" -lm";
		auto lr = runProcess(pImpl->avrGcc, linkArgs.str(), pImpl->buildPath);
		if (lr.exitCode != 0) {
			pImpl->lastError = "Link failed:\n" + lr.error;
			return false;
		}

		// ---- avr-objcopy で .hex 生成 ----
		std::ostringstream objArgs;
		objArgs << "-O ihex -R .eeprom "
			<< "\"" << pImpl->elfFile << "\" "
			<< "\"" << pImpl->hexFile << "\"";
		auto orr = runProcess(pImpl->avrObjcopy, objArgs.str(), pImpl->buildPath);
		if (orr.exitCode != 0) {
			pImpl->lastError = "Objcopy failed:\n" + orr.error;
			return false;
		}

		Utils::writeFile(pImpl->hashCacheFile, pImpl->currentSignature);
		return true;
	} catch (const std::exception& e) {
		pImpl->lastError = std::string("Incremental compile error: ") + e.what();
		return false;
	}
}

bool Uploader::uploadToBoard() {
	// 指定されたポートに対して avrdude を 1 回実行するヘルパ
	auto runAvrdude = [&](const std::string& port) -> ProcessResult {
		std::ostringstream args;
		args << "-C \"" << pImpl->avrdudeConf << "\" -q -q -V "
			<< "-p " << kAvrdudeMcu
			<< " -c " << kAvrdudeProg
			<< " -P " << port
			<< " -b " << kAvrdudeBaud
			<< " -D -U flash:w:\"" << pImpl->hexFile << "\":i";
		return runProcess(pImpl->avrdude, args.str(), pImpl->buildPath);
		};

	// ポートが開けない場合の擬似結果(avrdude を呼ばずに失敗扱いにするため)
	auto makeNotOpenableResult = [](const std::string& port) {
		ProcessResult r;
		r.exitCode = -1;
		r.error = "Port " + port + " is not openable (device disconnected?)\n";
		return r;
		};

	try {
		// --- 1 回目: 現在の targetPort で書き込み ---
		ProcessResult result;
		if (!comPortIsOpenable(pImpl->targetPort)) {
			// ポートが存在しない/開けない場合は avrdude を呼ばずに即失敗
			// (avrdude は失敗時に約 7 秒の stk500 sync リトライを行うので回避する)
			result = makeNotOpenableResult(pImpl->targetPort);
		} else {
			result = runAvrdude(pImpl->targetPort);
		}

		if (result.exitCode == 0) {
			Utils::writeFileIfChanged(pImpl->portCacheFile, pImpl->targetPort);
			return true;
		}

		// --- 失敗時の対応 ---
		// ユーザーが明示指定したポートで失敗した場合は、勝手に他のポートを
		// 試さずにそのままエラー終了する(ユーザーの意思を尊重)。
		if (!pImpl->requestedPort.empty()) {
			pImpl->lastError = "Upload failed on port " + pImpl->targetPort + ":\n" + result.error;
			return false;
		}

		// 自動探索モード: キャッシュ済みのポートが動かなくなった可能性があるので、
		// 再探索して別のポートを試す。
		std::cout << "Upload failed on " << pImpl->targetPort
			<< ", searching for another COM port...\n";

		std::string newPort = findArduinoPort();
		if (newPort.empty()) {
			// ポートが見つからない: キャッシュは破棄して終了
			Utils::deleteFile(pImpl->portCacheFile);
			pImpl->lastError = "Upload failed and no other COM port found:\n" + result.error;
			return false;
		}

		if (newPort == pImpl->targetPort) {
			// 同じポートしか見つからない -> リトライしても無駄なので失敗扱い
			// (キャッシュは正しいので残す)
			pImpl->lastError = "Upload failed on port " + pImpl->targetPort
				+ " (no alternative port available):\n" + result.error;
			return false;
		}

		// --- 2 回目: 新しく見つかったポートで再試行(同じく事前チェック付き) ---
		std::cout << "Retrying with port " << newPort << "...\n";
		pImpl->targetPort = newPort;

		ProcessResult retry;
		if (!comPortIsOpenable(newPort)) {
			retry = makeNotOpenableResult(newPort);
		} else {
			retry = runAvrdude(newPort);
		}

		if (retry.exitCode != 0) {
			Utils::deleteFile(pImpl->portCacheFile);
			pImpl->lastError = "Upload failed on port " + newPort + ":\n" + retry.error;
			return false;
		}

		Utils::writeFileIfChanged(pImpl->portCacheFile, newPort);
		return true;
	} catch (const std::exception& e) {
		pImpl->lastError = std::string("Upload error: ") + e.what();
		return false;
	}
}

// =============================================================================
// パス解決ヘルパ
// =============================================================================
std::string Uploader::resolveCachedDirectory(const std::string& cacheFile,
	const std::string& baseDir,
	const std::string& errorMessage,
	const std::string& suffix) {
	// キャッシュを試す
	if (Utils::fileExists(cacheFile)) {
		try {
			std::string cached = Utils::trim(Utils::readFile(cacheFile));
			if (Utils::directoryExists(cached)) {
				return cached;
			}
		} catch (...) {
			// 続行(再検索)
		}
	}

	// 最新ディレクトリを探す
	std::string latest = Utils::getLatestDirectory(baseDir);
	if (latest.empty()) {
		throw std::runtime_error(errorMessage);
	}

	std::string result = latest + suffix;
	Utils::writeFileIfChanged(cacheFile, result);
	return result;
}

bool Uploader::resolveAvrdudePaths() {
	try {
		// キャッシュを試す
		if (Utils::fileExists(pImpl->avrdudeCacheFile)) {
			try {
				auto lines = Utils::readLines(pImpl->avrdudeCacheFile);
				if (lines.size() >= 2 &&
					Utils::fileExists(lines[0]) &&
					Utils::fileExists(lines[1])) {
					pImpl->avrdude = lines[0];
					pImpl->avrdudeConf = lines[1];
					return true;
				}
			} catch (...) {
				// フォールスルー
			}
		}

		std::string latest = Utils::getLatestDirectory(
			Utils::joinPath(Utils::getLocalAppDataPath(), kAvrdudeBase));
		if (latest.empty()) {
			pImpl->lastError = "avrdude not found";
			return false;
		}

		pImpl->avrdude = Utils::joinPath(latest, "bin\\avrdude.exe");
		pImpl->avrdudeConf = Utils::joinPath(latest, "etc\\avrdude.conf");

		if (!Utils::fileExists(pImpl->avrdude) || !Utils::fileExists(pImpl->avrdudeConf)) {
			pImpl->lastError = "avrdude files not found";
			return false;
		}

		Utils::writeLines(pImpl->avrdudeCacheFile, { pImpl->avrdude, pImpl->avrdudeConf });
		return true;
	} catch (const std::exception& e) {
		pImpl->lastError = std::string("Avrdude path resolution error: ") + e.what();
		return false;
	}
}

// =============================================================================
// COM ポート検出 (WMI)
// =============================================================================
namespace {

	// RAII で COM 終了処理。
	// COINIT_MULTITHREADED で初期化を試み、別スレッドやシェル系 API などで
	// 既に APARTMENTTHREADED で初期化されている場合(RPC_E_CHANGED_MODE)は
	// それに合わせて再初期化する。S_FALSE(既に同じモードで初期化済み)も成功扱い。
	class ComScope {
	public:
		ComScope() {
			HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
			if (hr == RPC_E_CHANGED_MODE) {
				hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
			}
			ok_ = SUCCEEDED(hr);  // S_OK と S_FALSE はどちらも成功
			// どちらの結果でも、対応する CoUninitialize を呼ぶ必要がある
		}
		~ComScope() { if (ok_) CoUninitialize(); }
		bool initialized() const { return ok_; }
	private:
		bool ok_ = false;
	};

	// WBSTR -> std::string (UTF-8 変換)
	std::string bstrToUtf8(BSTR bstr) {
		if (!bstr) return {};
		int len = WideCharToMultiByte(CP_UTF8, 0, bstr, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) return {};
		std::string out(len - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, bstr, -1, out.data(), len, nullptr, nullptr);
		return out;
	}

	// "...(COMxx)" の形式から "COMxx" を抜き出す
	std::string extractComPort(const std::string& caption) {
		size_t open = caption.rfind("(COM");
		if (open == std::string::npos) return {};
		size_t close = caption.find(')', open);
		if (close == std::string::npos) return {};
		return caption.substr(open + 1, close - open - 1);
	}

	bool matchesAnyPattern(const std::string& s, const std::vector<std::string>& patterns) {
		for (const auto& p : patterns) {
			if (s.find(p) != std::string::npos) return true;
		}
		return false;
	}

	// WMI が失敗した場合のフォールバック:
	// レジストリ HKLM\HARDWARE\DEVICEMAP\SERIALCOMM から接続中の COM ポートを列挙する。
	// Windows が認識しているすべてのシリアルポートが入っているので、
	// Arduino が刺さっていれば必ずここに現れる(ドライバ名は分からないが COM 番号は分かる)。
	std::vector<std::string> enumComPortsFromRegistry() {
		std::vector<std::string> ports;
		HKEY hKey = nullptr;
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
			"HARDWARE\\DEVICEMAP\\SERIALCOMM",
			0, KEY_READ, &hKey) != ERROR_SUCCESS) {
			return ports;
		}

		DWORD index = 0;
		char valueName[256];
		BYTE  valueData[256];
		while (true) {
			DWORD nameLen = sizeof(valueName);
			DWORD dataLen = sizeof(valueData);
			DWORD type = 0;
			LONG  result = RegEnumValueA(hKey, index, valueName, &nameLen, nullptr,
				&type, valueData, &dataLen);
			if (result == ERROR_NO_MORE_ITEMS) break;
			if (result != ERROR_SUCCESS) break;

			if (type == REG_SZ && dataLen > 0) {
				// valueData は \0 終端の COM 名("COM3" 等)
				ports.emplace_back(reinterpret_cast<const char*>(valueData));
			}
			++index;
		}
		RegCloseKey(hKey);
		return ports;
	}

	// 指定された COM ポートが現在 OS で開けるかを即座に確認する。
	// avrdude を呼ぶ前にこれで弾けば、失敗時の長い stk500 タイムアウト
	// (約 7 秒)を完全に回避できる。
	bool comPortIsOpenable(const std::string& port) {
		// "\\.\COM10" のような形式にしないと 2 桁以上の COM 番号が開けない
		std::string path = "\\\\.\\" + port;
		HANDLE h = CreateFileA(path.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			0,            // 排他アクセス
			nullptr,
			OPEN_EXISTING,
			0,
			nullptr);
		if (h == INVALID_HANDLE_VALUE) {
			return false;
		}
		CloseHandle(h);
		return true;
	}

	// WMI 経由で COM ポートを探す。失敗/見つからなかった場合は空文字を返す。
	// allCandidates には(成功したかに関わらず)WMI が列挙した COM デバイス名が入る。
	std::string findArduinoPortViaWmi(std::vector<std::string>& allCandidates) {
		auto hexHr = [](HRESULT h) {
			std::ostringstream o;
			o << "0x" << std::hex << std::uppercase
				<< static_cast<unsigned long>(static_cast<unsigned int>(h));
			return o.str();
			};

		ComScope com;
		if (!com.initialized()) {
			// std::cout << "[port-search] CoInitializeEx failed\n";
			return "";
		}

		// セキュリティレベル(失敗しても続行可能なケースがあるので結果を見ない)
		CoInitializeSecurity(nullptr, -1, nullptr, nullptr,
			RPC_C_AUTHN_LEVEL_DEFAULT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr, EOAC_NONE, nullptr);

		IWbemLocator* pLoc = nullptr;
		HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr,
			CLSCTX_INPROC_SERVER, IID_IWbemLocator,
			reinterpret_cast<LPVOID*>(&pLoc));
		if (FAILED(hr)) {
			// std::cout << "[port-search] CoCreateInstance(WbemLocator) failed: " << hexHr(hr) << "\n";
			return "";
		}

		IWbemServices* pSvc = nullptr;
		hr = pLoc->ConnectServer(ComBStr(L"ROOT\\CIMV2"),
			nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
		if (FAILED(hr)) {
			// std::cout << "[port-search] ConnectServer(ROOT\\CIMV2) failed: " << hexHr(hr) << "\n";
			pLoc->Release();
			return "";
		}

		hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
			RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
			nullptr, EOAC_NONE);
		if (FAILED(hr)) {
			// std::cout << "[port-search] CoSetProxyBlanket failed: " << hexHr(hr) << "\n";
			pSvc->Release(); pLoc->Release();
			return "";
		}

		IEnumWbemClassObject* pEnum = nullptr;
		hr = pSvc->ExecQuery(ComBStr(L"WQL"),
			ComBStr(kWmiQuery),
			WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
			nullptr, &pEnum);
		if (FAILED(hr)) {
			// std::cout << "[port-search] ExecQuery failed: " << hexHr(hr) << "\n";
			pSvc->Release(); pLoc->Release();
			return "";
		}

		std::string foundPort;
		while (pEnum) {
			IWbemClassObject* pObj = nullptr;
			ULONG returned = 0;
			if (FAILED(pEnum->Next(WBEM_INFINITE, 1, &pObj, &returned)) || returned == 0) {
				break;
			}

			VARIANT vt; VariantInit(&vt);
			if (SUCCEEDED(pObj->Get(L"Name", 0, &vt, nullptr, nullptr)) && vt.vt == VT_BSTR) {
				std::string name = bstrToUtf8(vt.bstrVal);
				allCandidates.push_back(name);
				if (foundPort.empty() && matchesAnyPattern(name, kSerialDevicePatterns)) {
					foundPort = extractComPort(name);
				}
			}
			VariantClear(&vt);
			pObj->Release();
			// 早期 break しない: 全候補を集める
		}

		// 候補が 1 つだけなら(パターン一致しなくても)それを使う
		if (foundPort.empty() && allCandidates.size() == 1) {
			foundPort = extractComPort(allCandidates[0]);
		}

		pEnum->Release();
		pSvc->Release();
		pLoc->Release();
		return foundPort;
	}

} // namespace

std::string Uploader::findArduinoPort() {
	// ステップ 1: WMI で検索
	std::vector<std::string> wmiCandidates;
	std::string foundPort = findArduinoPortViaWmi(wmiCandidates);
	if (!foundPort.empty()) {
		return foundPort;
	}

	// ステップ 2: レジストリ HKLM\HARDWARE\DEVICEMAP\SERIALCOMM をフォールバック
	auto regPorts = enumComPortsFromRegistry();
	if (regPorts.empty()) {
		if (wmiCandidates.empty()) {
			std::cout << "(WMI and registry both returned no COM devices)\n";
		} else {
			std::cout << "WMI listed COM devices but none matched, "
				"and registry returned nothing:\n";
			for (const auto& c : wmiCandidates) {
				std::cout << "  - " << c << "\n";
			}
		}
		return "";
	}

	// COM1 / COM2 は通常 PC 内蔵のシリアル(BIOS, RS-232, Bluetooth 等)であり、
	// Arduino である可能性は事実上ない。これらに書き込みを試行すると stk500 の
	// タイムアウトで数秒待たされるので、自動探索からは完全に除外する。
	// (本当に COM1/COM2 を使いたい場合は tasks.json の args で明示指定する。)
	auto isLikelyBuiltin = [](const std::string& p) {
		return p == "COM1" || p == "COM2";
		};
	regPorts.erase(
		std::remove_if(regPorts.begin(), regPorts.end(), isLikelyBuiltin),
		regPorts.end());

	if (regPorts.empty()) {
		// std::cout << "[port-search] Only built-in COM ports (COM1/COM2) are available; "
			// "no Arduino-like port detected.\n";
		return "";
	}

	// std::cout << "[port-search] Using registry-listed COM ports:\n";
	for (const auto& p : regPorts) {
		std::cout << "  - " << p << "\n";
	}

	// ステップ 2a: WMI の候補にドライバ名でマッチするものがあればそれを最優先
	for (const auto& cand : wmiCandidates) {
		if (matchesAnyPattern(cand, kSerialDevicePatterns)) {
			std::string p = extractComPort(cand);
			if (!p.empty() && !isLikelyBuiltin(p)) return p;
		}
	}

	// ステップ 2b: 残ったポートの先頭を使う
	return regPorts.front();
}