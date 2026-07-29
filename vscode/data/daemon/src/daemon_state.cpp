#include "daemon_state.h"
#include "utils.h"
#include "port_scanner.h"

#include <windows.h>
#include <cctype>
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

	void loadPredefinedMacros(ToolchainPaths& tc) {
		const std::string args =
			"-dM -E -x c++ -std=gnu++11 -mmcu=atmega2560"
			" -DF_CPU=16000000L -DARDUINO=10819"
			" -DARDUINO_AVR_MEGA2560 -DARDUINO_ARCH_AVR NUL";
		ProcessResult result = runProcess(tc.avrGpp, args, "", true);
		if (result.exitCode == 0) {
			for (const auto& line : Utils::split(result.output, '\n')) {
				static const std::string prefix = "#define ";
				if (line.compare(0, prefix.size(), prefix) != 0) continue;
				const size_t nameStart = prefix.size();
				size_t nameEnd = nameStart;
				while (nameEnd < line.size()
					&& !std::isspace(static_cast<unsigned char>(line[nameEnd]))
					&& line[nameEnd] != '(') {
					++nameEnd;
				}
				if (nameEnd == nameStart) continue;
				const std::string name =
					line.substr(nameStart, nameEnd - nameStart);
				if (nameEnd < line.size() && line[nameEnd] == '(') {
					tc.predefinedFunctionMacros[name] =
						Utils::trim(line.substr(nameEnd));
				}
				else {
					const std::string value =
						Utils::trim(line.substr(nameEnd));
					tc.predefinedMacros[name] =
						value.empty() ? "1" : value;
				}
			}
		}
		// コンパイラ照会に失敗しても、拡張機能が必ず渡す定義は保証する。
		tc.predefinedMacros["F_CPU"] = "16000000L";
		tc.predefinedMacros["ARDUINO"] = "10819";
		tc.predefinedMacros["ARDUINO_AVR_MEGA2560"] = "1";
		tc.predefinedMacros["ARDUINO_ARCH_AVR"] = "1";
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
		fs::path resourceRoot = Utils::pathFromUtf8(extensionRoot)
			/ L"resources" / L"arduino";
		bundledGccRoot = Utils::pathToUtf8(resourceRoot / L"avr-gcc");
		bundledHardwareRoot = Utils::pathToUtf8(resourceRoot / L"hardware");
		tc.librariesDir = Utils::pathToUtf8(resourceRoot / L"libraries");
		tc.prebuiltCoreA = Utils::pathToUtf8(resourceRoot / L"prebuilt" / L"core.a");
	}

	// 1) avr-gcc のディレクトリ
	const bool hasBundledGcc = Utils::directoryExists(bundledGccRoot);
	std::string gccRoot = hasBundledGcc
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
	const bool hasBundledHardware =
		Utils::directoryExists(bundledHardwareRoot);
	if (hasBundledHardware) {
		const fs::path hardwareRoot = Utils::pathFromUtf8(bundledHardwareRoot);
		tc.coreDir = Utils::pathToUtf8(hardwareRoot / L"cores" / L"arduino");
		tc.variantDir = Utils::pathToUtf8(hardwareRoot / L"variants" / L"mega");
	} else {
		tc.coreDir = findArduinoCoreDir();
		tc.variantDir = findVariantDir("mega");
		if (!tc.coreDir.empty()) {
			const fs::path hardwareVersion =
				Utils::pathFromUtf8(tc.coreDir).parent_path().parent_path();
			tc.librariesDir =
				Utils::pathToUtf8(hardwareVersion / L"libraries");
		}
	}
	if (tc.coreDir.empty() || !Utils::directoryExists(tc.coreDir)) {
		tc.errorMessage = "Arduino core directory not found";
		return false;
	}
	if (tc.variantDir.empty() || !Utils::directoryExists(tc.variantDir)) {
		tc.errorMessage = "Arduino Mega variant directory not found";
		return false;
	}
	if (Utils::fileExists(tc.prebuiltCoreA)) {
		try {
			tc.prebuiltCoreHash = Utils::sha1FileHex(tc.prebuiltCoreA);
			if (tc.prebuiltCoreHash.empty()) {
				throw std::runtime_error("core.a hashing failed");
			}
		}
		catch (const std::exception& e) {
			tc.errorMessage = "Cannot read bundled core.a: " + std::string(e.what());
			return false;
		}
	}

	// 4) コンパイラバージョン
	tc.compilerVersion = getCompilerVersionString(tc.avrGcc);
	loadPredefinedMacros(tc);
	tc.usingBundledResources = hasBundledGcc && hasBundledHardware;

	// 5) core.a キャッシュルート
	g_state.coreCacheRoot = cacheRoot.empty()
		? Utils::joinPath(Utils::getGlobalCacheDir(), "core-cache")
		: Utils::joinPath(cacheRoot, "cores");
	Utils::createDirectory(g_state.coreCacheRoot);

	// 6) 起動時刻 / COM ポート初回スキャン
	g_state.startedAt = std::chrono::steady_clock::now();
	{
		std::lock_guard<std::mutex> lock(g_state.activityMtx);
		g_state.lastRequestAt = g_state.startedAt;
	}
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
