#include "daemon_state.h"
#include "utils.h"
#include "port_scanner.h"

#include <windows.h>
#include <filesystem>

namespace fs = std::filesystem;

DaemonState g_state;

namespace {

	// %LOCALAPPDATA%\Arduino15\packages\arduino\tools\arm-none-eabi-gcc\<version>
	std::string findLatestArmToolchain() {
		// 新しい renesas_uno パッケージで使われる現行の配置
		std::string base = Utils::joinPath(Utils::getLocalAppDataPath(),
			"Arduino15\\packages\\arduino\\tools\\arm-none-eabi-gcc");
		if (Utils::directoryExists(base)) {
			std::string latest = Utils::getLatestDirectory(base);
			if (!latest.empty()) return latest;
		}

		// 過去・現在の Arduino renesas パッケージで使われる別配置 (xpack)
		// 例: tools\xpack-arm-none-eabi-gcc\<version>
		std::vector<std::string> alts = {
			Utils::joinPath(Utils::getLocalAppDataPath(),
				"Arduino15\\packages\\arduino\\tools\\xpack-arm-none-eabi-gcc"),
			Utils::joinPath(Utils::getLocalAppDataPath(),
				"Arduino15\\packages\\arduino\\tools\\gcc-arm-none-eabi"),
		};
		for (const auto& a : alts) {
			if (Utils::directoryExists(a)) {
				std::string latest = Utils::getLatestDirectory(a);
				if (!latest.empty()) return latest;
			}
		}
		return "";
	}

	// dfu-util 配置候補:
	//   %LOCALAPPDATA%\Arduino15\packages\arduino\tools\dfu-util\<ver>\dfu-util.exe
	//   (互換クローン用に PATH 上の dfu-util.exe も拾う)
	std::string findDfuUtil() {
		std::string base = Utils::joinPath(Utils::getLocalAppDataPath(),
			"Arduino15\\packages\\arduino\\tools\\dfu-util");
		if (Utils::directoryExists(base)) {
			std::string verDir = Utils::getLatestDirectory(base);
			if (!verDir.empty()) {
				// dfu-util バンドルは <ver>\dfu-util.exe または <ver>\dfu-util-static.exe
				std::vector<std::string> candidates = {
					Utils::joinPath(verDir, "dfu-util.exe"),
					Utils::joinPath(verDir, "dfu-util-static.exe"),
					Utils::joinPath(verDir, "bin\\dfu-util.exe"),
				};
				for (const auto& c : candidates) {
					if (Utils::fileExists(c)) return c;
				}
			}
		}

		// PATH 上の dfu-util.exe
		char buf[MAX_PATH * 2];
		DWORD r = SearchPathA(nullptr, "dfu-util.exe", nullptr, sizeof(buf), buf, nullptr);
		if (r > 0 && r < sizeof(buf)) return std::string(buf, r);

		return "";
	}

	// renesas_uno ハードウェアパッケージのルート (バージョン付き) を返す
	std::string findRenesasHardwareRoot() {
		std::string base = Utils::joinPath(Utils::getLocalAppDataPath(),
			"Arduino15\\packages\\arduino\\hardware\\renesas_uno");
		if (!Utils::directoryExists(base)) return "";
		return Utils::getLatestDirectory(base);
	}

	std::string findArduinoCoreDir(const std::string& hwRoot) {
		if (hwRoot.empty()) return "";
		return Utils::joinPath(hwRoot, "cores\\arduino");
	}

	std::string findVariantDir(const std::string& hwRoot, const std::string& variantName) {
		if (hwRoot.empty()) return "";
		return Utils::joinPath(hwRoot, "variants\\" + variantName);
	}

	std::string findLinkerScript(const std::string& variantDir) {
		// 典型は <variant>\linker_scripts\gcc\fsp.ld
		std::vector<std::string> candidates = {
			Utils::joinPath(variantDir, "linker_scripts\\gcc\\fsp.ld"),
			Utils::joinPath(variantDir, "linker_script.ld"),
			Utils::joinPath(variantDir, "fsp.ld"),
		};
		for (const auto& c : candidates) {
			if (Utils::fileExists(c)) return c;
		}
		return "";
	}

	// renesas_uno 用の追加 include ディレクトリを ";" 区切りで返す。
	// FSP / RA / e2studio などの大量のヘッダディレクトリを束ねる。
	std::string collectExtraIncludes(const std::string& hwRoot,
		const std::string& variantDir) {
		std::vector<std::string> dirs;
		if (!variantDir.empty() && Utils::directoryExists(variantDir)) {
			dirs.push_back(variantDir);
			// variants/UNOWIFIR4 配下にある主要 include 候補
			std::vector<std::string> sub = {
				"includes",
				"includes/api",
				"includes/api/deprecated",
				"includes/api/deprecated-avr-comp",
			};
			for (const auto& s : sub) {
				std::string p = Utils::joinPath(variantDir, s);
				if (Utils::directoryExists(p)) dirs.push_back(p);
			}
		}

		// cores/arduino/api などコア側の追加 include
		std::string coreDir = Utils::joinPath(hwRoot, "cores\\arduino");
		if (Utils::directoryExists(coreDir)) {
			std::vector<std::string> sub = {
				"api",
				"api/deprecated",
				"api/deprecated-avr-comp",
			};
			for (const auto& s : sub) {
				std::string p = Utils::joinPath(coreDir, s);
				if (Utils::directoryExists(p)) dirs.push_back(p);
			}
		}

		// FSP (Renesas Flexible Software Package) は別パッケージとしてバンドルされる
		// 典型: packages/arduino/tools/fsp/<ver>/...
		std::string fspRoot = Utils::joinPath(Utils::getLocalAppDataPath(),
			"Arduino15\\packages\\arduino\\tools\\fsp");
		if (Utils::directoryExists(fspRoot)) {
			std::string verDir = Utils::getLatestDirectory(fspRoot);
			if (!verDir.empty() && Utils::directoryExists(verDir)) {
				// FSP は多数の include サブディレクトリを抱える。ここでは網羅せず、
				// 代表的なルートを通しておけば残りはコアの cores/arduino/IRQManager 等が
				// 相対 include で解決する。
				dirs.push_back(verDir);
				for (const auto& s : { "include", "ra", "ra_gen", "ra_cfg" }) {
					std::string p = Utils::joinPath(verDir, s);
					if (Utils::directoryExists(p)) dirs.push_back(p);
				}
			}
		}

		std::string joined;
		for (const auto& d : dirs) {
			if (!joined.empty()) joined += ";";
			joined += d;
		}
		return joined;
	}

	std::string getCompilerVersionString(const std::string& gccPath) {
		ProcessResult r = runProcess(gccPath, "--version", "", true);
		if (r.exitCode != 0) return "unknown";
		// 最初の行だけ
		size_t nl = r.output.find('\n');
		std::string line = (nl == std::string::npos) ? r.output : r.output.substr(0, nl);
		return Utils::trim(line);
	}

	std::string extractRenesasVersion(const std::string& hwRoot) {
		if (hwRoot.empty()) return "";
		return fs::path(hwRoot).filename().string();
	}

} // namespace

bool initializeDaemonState() {
	auto& tc = g_state.toolchain;

	// 1) arm-none-eabi-gcc のディレクトリ
	std::string gccRoot = findLatestArmToolchain();
	if (gccRoot.empty()) {
		tc.errorMessage = "arm-none-eabi-gcc not found under %LOCALAPPDATA%\\Arduino15"
			" (install Arduino UNO R4 board package via Boards Manager)";
		return false;
	}
	tc.armGpp = Utils::joinPath(gccRoot, "bin\\arm-none-eabi-g++.exe");
	tc.armGcc = Utils::joinPath(gccRoot, "bin\\arm-none-eabi-gcc.exe");
	tc.armObjcopy = Utils::joinPath(gccRoot, "bin\\arm-none-eabi-objcopy.exe");
	tc.armAr = Utils::joinPath(gccRoot, "bin\\arm-none-eabi-ar.exe");
	tc.armSize = Utils::joinPath(gccRoot, "bin\\arm-none-eabi-size.exe");

	for (const auto& p : { tc.armGpp, tc.armGcc, tc.armObjcopy, tc.armAr }) {
		if (!Utils::fileExists(p)) {
			tc.errorMessage = "Toolchain binary missing: " + p;
			return false;
		}
	}

	// 2) dfu-util (アップロード用)
	tc.dfuUtil = findDfuUtil();
	if (tc.dfuUtil.empty()) {
		tc.errorMessage = "dfu-util not found "
			"(expected under %LOCALAPPDATA%\\Arduino15\\packages\\arduino\\tools\\dfu-util or in PATH)";
		return false;
	}

	// 3) Arduino renesas_uno コア + variant
	std::string hwRoot = findRenesasHardwareRoot();
	if (hwRoot.empty()) {
		tc.errorMessage = "Arduino renesas_uno hardware package not found"
			" (install via Boards Manager: 'Arduino UNO R4 Boards')";
		return false;
	}
	tc.boardPackageVersion = extractRenesasVersion(hwRoot);

	tc.coreDir = findArduinoCoreDir(hwRoot);
	if (tc.coreDir.empty() || !Utils::directoryExists(tc.coreDir)) {
		tc.errorMessage = "Arduino core directory not found under " + hwRoot;
		return false;
	}

	// 4) UNO R4 WiFi 用の variant
	tc.variantDir = findVariantDir(hwRoot, "UNOWIFIR4");
	if (tc.variantDir.empty() || !Utils::directoryExists(tc.variantDir)) {
		tc.errorMessage = "variant UNOWIFIR4 not found under " + hwRoot;
		return false;
	}

	// 5) リンカスクリプト
	tc.linkerScript = findLinkerScript(tc.variantDir);
	// 無くてもエラーにはしない (arduino-cli 経由の core.a 生成は走るため)

	// 6) FSP / variant 追加 include
	tc.includeDirs = collectExtraIncludes(hwRoot, tc.variantDir);

	// 7) コンパイラバージョン
	tc.compilerVersion = getCompilerVersionString(tc.armGcc);

	// 8) core.a キャッシュルート
	g_state.coreCacheRoot = Utils::joinPath(Utils::getGlobalCacheDir(), "core-cache");
	Utils::createDirectory(g_state.coreCacheRoot);

	// 9) 起動時刻 / COM ポート初回スキャン
	g_state.startedAt = std::chrono::steady_clock::now();
	g_state.lastRequestAt = g_state.startedAt;
	refreshComPorts();

	tc.valid = true;
	return true;
}

void refreshComPorts() {
	auto names = PortScanner::listNames();
	auto arduino = PortScanner::findArduinoPort();
	std::lock_guard<std::mutex> lk(g_state.portMtx);
	g_state.cachedPorts = std::move(names);
	g_state.cachedArduinoPort = std::move(arduino);
	g_state.portsCachedAt = std::chrono::steady_clock::now();
}
