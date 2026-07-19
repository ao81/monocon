#include "daemon_state.h"
#include "utils.h"
#include "port_scanner.h"

#include <windows.h>

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

bool initializeDaemonState() {
	auto& tc = g_state.toolchain;

	// 1) avr-gcc のディレクトリ
	std::string gccRoot = findLatestArduinoToolchain();
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

	// 2) avrdude
	std::string avrdudeRoot = findLatestAvrdude();
	if (avrdudeRoot.empty()) {
		tc.errorMessage = "avrdude not found";
		return false;
	}
	tc.avrdude = Utils::joinPath(avrdudeRoot, "bin\\avrdude.exe");
	tc.avrdudeConf = Utils::joinPath(avrdudeRoot, "etc\\avrdude.conf");
	if (!Utils::fileExists(tc.avrdude) || !Utils::fileExists(tc.avrdudeConf)) {
		tc.errorMessage = "avrdude binary or config missing";
		return false;
	}

	// 3) Arduino コア
	tc.coreDir = findArduinoCoreDir();
	if (tc.coreDir.empty() || !Utils::directoryExists(tc.coreDir)) {
		tc.errorMessage = "Arduino core directory not found";
		return false;
	}
	// MEGA 用のバリアントディレクトリ (デフォルト)
	tc.variantDir = findVariantDir("mega");

	// 4) コンパイラバージョン
	tc.compilerVersion = getCompilerVersionString(tc.avrGcc);

	// 5) core.a キャッシュルート
	g_state.coreCacheRoot = Utils::joinPath(Utils::getGlobalCacheDir(), "core-cache");
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
