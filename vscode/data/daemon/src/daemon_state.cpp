#include "daemon_state.h"
#include "utils.h"
#include "port_scanner.h"

#include <windows.h>
#include <filesystem>

namespace fs = std::filesystem;

DaemonState g_state;

namespace {

	// %LOCALAPPDATA%\Arduino15\packages\arduino\tools\avr-gcc\<version>
	std::string findLatestArduinoToolchain() {
		std::string base = Utils::joinPath(Utils::getLocalAppDataPath(),
			"Arduino15\\packages\\arduino\\tools\\avr-gcc");
		if (!Utils::directoryExists(base)) return "";
		return Utils::getLatestDirectory(base);
	}

	std::string findLatestAvrdude() {
		std::string base = Utils::joinPath(Utils::getLocalAppDataPath(),
			"Arduino15\\packages\\arduino\\tools\\avrdude");
		if (!Utils::directoryExists(base)) return "";
		return Utils::getLatestDirectory(base);
	}

	std::string findArduinoCoreDir() {
		// Arduino15\packages\arduino\hardware\avr\<version>\cores\arduino
		std::string base = Utils::joinPath(Utils::getLocalAppDataPath(),
			"Arduino15\\packages\\arduino\\hardware\\avr");
		if (!Utils::directoryExists(base)) return "";
		std::string verDir = Utils::getLatestDirectory(base);
		if (verDir.empty()) return "";
		return Utils::joinPath(verDir, "cores\\arduino");
	}

	std::string findVariantDir(const std::string& variantName) {
		std::string base = Utils::joinPath(Utils::getLocalAppDataPath(),
			"Arduino15\\packages\\arduino\\hardware\\avr");
		std::string verDir = Utils::getLatestDirectory(base);
		if (verDir.empty()) return "";
		return Utils::joinPath(verDir, "variants\\" + variantName);
	}

	std::string getCompilerVersionString(const std::string& avrGccPath) {
		ProcessResult r = runProcess(avrGccPath, "--version", "", true);
		if (r.exitCode != 0) return "unknown";
		// 最初の行だけ
		size_t nl = r.output.find('\n');
		std::string line = (nl == std::string::npos) ? r.output : r.output.substr(0, nl);
		return Utils::trim(line);
	}

} // namespace

bool initializeDaemonState(const std::string& extensionRoot,
	const std::string& cacheRoot) {
	auto& tc = g_state.toolchain;
	tc = ToolchainPaths{};
	g_state.buildCacheRoot = cacheRoot;

	std::string bundledGccRoot;
	std::string bundledHardwareRoot;
	if (!extensionRoot.empty()) {
		fs::path resourceRoot = fs::path(extensionRoot) / "resources" / "arduino";
		bundledGccRoot = (resourceRoot / "avr-gcc").string();
		bundledHardwareRoot = (resourceRoot / "hardware").string();
		tc.prebuiltCoreA = (resourceRoot / "prebuilt" / "core.a").string();
	}

	// 1) avr-gcc のディレクトリ
	std::string gccRoot = Utils::directoryExists(bundledGccRoot)
		? bundledGccRoot : findLatestArduinoToolchain();
	if (gccRoot.empty()) {
		tc.errorMessage = "avr-gcc not found under %LOCALAPPDATA%\\Arduino15";
		return false;
	}
	tc.avrGpp = Utils::joinPath(gccRoot, "bin\\avr-g++.exe");
	tc.avrGcc = Utils::joinPath(gccRoot, "bin\\avr-gcc.exe");
	tc.avrObjcopy = Utils::joinPath(gccRoot, "bin\\avr-objcopy.exe");
	tc.avrAr = Utils::joinPath(gccRoot, "bin\\avr-ar.exe");

	for (const auto& p : { tc.avrGpp, tc.avrGcc, tc.avrObjcopy, tc.avrAr }) {
		if (!Utils::fileExists(p)) {
			tc.errorMessage = "Toolchain binary missing: " + p;
			return false;
		}
	}

	// 2) avrdude（互換情報として任意で解決。ネイティブ書き込みには不要）
	std::string avrdudeRoot = findLatestAvrdude();
	if (!avrdudeRoot.empty()) {
		tc.avrdude = Utils::joinPath(avrdudeRoot, "bin\\avrdude.exe");
		tc.avrdudeConf = Utils::joinPath(avrdudeRoot, "etc\\avrdude.conf");
	}

	// 3) Arduino コア
	if (Utils::directoryExists(bundledHardwareRoot)) {
		tc.coreDir = (fs::path(bundledHardwareRoot) / "cores" / "arduino").string();
		tc.variantDir = (fs::path(bundledHardwareRoot) / "variants" / "mega").string();
	} else {
		tc.coreDir = findArduinoCoreDir();
		tc.variantDir = findVariantDir("mega");
	}
	if (tc.coreDir.empty() || !Utils::directoryExists(tc.coreDir)) {
		tc.errorMessage = "Arduino core directory not found";
		return false;
	}
	if (tc.variantDir.empty() || !Utils::directoryExists(tc.variantDir)) {
		tc.errorMessage = "Arduino Mega variant directory not found";
		return false;
	}

	// 4) コンパイラバージョン
	tc.compilerVersion = getCompilerVersionString(tc.avrGcc);

	// 5) core.a キャッシュルート
	g_state.coreCacheRoot = cacheRoot.empty()
		? Utils::joinPath(Utils::getGlobalCacheDir(), "core-cache")
		: Utils::joinPath(cacheRoot, "cores");
	Utils::createDirectory(g_state.coreCacheRoot);

	// 6) 起動時刻 / COM ポート初回スキャン
	g_state.startedAt = std::chrono::steady_clock::now();
	g_state.lastRequestAt = g_state.startedAt;
	refreshComPorts();

	tc.valid = true;
	return true;
}

void refreshComPorts() {
	auto names = PortScanner::listNames();
	auto arduino = PortScanner::findArduinoPort();
	{
		std::lock_guard<std::mutex> lk(g_state.portMtx);
		g_state.cachedPorts = std::move(names);
		g_state.cachedArduinoPort = std::move(arduino);
		g_state.portsCachedAt = std::chrono::steady_clock::now();
	}
}
