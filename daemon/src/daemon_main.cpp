#include "daemon_state.h"
#include "pipe_io.h"
#include "builder.h"
#include "utils.h"
#include "port_scanner.h"

#include <windows.h>
#include <sddl.h>

#include <atomic>
#include <fstream>
#include <iostream>
#include <thread>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

#pragma comment(lib, "advapi32.lib")

// =============================================================================
// 簡易ロガー
// =============================================================================
namespace {
	std::mutex g_logMtx;
	std::ofstream g_logFile;
	bool g_foreground = false;

	void initLogger() {
		std::string logDir = Utils::getGlobalCacheDir();
		Utils::createDirectory(logDir);
		std::string logPath = Utils::joinPath(logDir, "daemon.log");
		g_logFile.open(logPath, std::ios::app);
	}

	void logLine(const std::string& level, const std::string& msg) {
		auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char tbuf[32];
		std::tm tmv;
		localtime_s(&tmv, &now);
		std::strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &tmv);
		std::string line = std::string("[") + tbuf + "][" + level + "] " + msg;
		std::lock_guard<std::mutex> lk(g_logMtx);
		if (g_logFile) g_logFile << line << '\n', g_logFile.flush();
		if (g_foreground) std::cout << line << '\n';
	}

	void logInfo(const std::string& m) { logLine("INFO", m); }
	void logWarn(const std::string& m) { logLine("WARN", m); }
	void logErr(const std::string& m) { logLine("ERR ", m); }
}

// =============================================================================
// 名前付きパイプの DACL: 同一ユーザーにのみ全権付与
// =============================================================================
static SECURITY_ATTRIBUTES* makePipeSecurity(SECURITY_ATTRIBUTES& sa,
	PSECURITY_DESCRIPTOR& pSD) {
	// "D:(A;;GA;;;OW)"  Owner にのみ Generic All
	if (!ConvertStringSecurityDescriptorToSecurityDescriptorA(
		"D:(A;;GA;;;OW)", SDDL_REVISION_1, &pSD, nullptr)) {
		return nullptr;
	}
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;
	return &sa;
}

// =============================================================================
// JSON-RPC ハンドラ
// =============================================================================
static json buildErrorResponse(const json& reqId, int code, const std::string& msg) {
	return json{
		{"jsonrpc", "2.0"},
		{"id", reqId},
		{"error", {{"code", code}, {"message", msg}}}
	};
}

static json buildOkResponse(const json& reqId, const json& result) {
	return json{
		{"jsonrpc", "2.0"},
		{"id", reqId},
		{"result", result}
	};
}

static json handlePing() {
	auto now = std::chrono::steady_clock::now();
	auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
		now - g_state.startedAt).count();
	return {
		{"version", g_state.version},
		{"uptimeSec", uptime},
		{"requestCount", g_state.requestCount.load()},
		{"toolchainValid", g_state.toolchain.valid},
		{"toolchainError", g_state.toolchain.errorMessage},
		{"compilerVersion", g_state.toolchain.compilerVersion}
	};
}

static json handleListPorts() {
	// 5 秒以上前のキャッシュなら更新
	auto now = std::chrono::steady_clock::now();
	bool needRefresh = false;
	{
		std::lock_guard<std::mutex> lk(g_state.portMtx);
		needRefresh = (now - g_state.portsCachedAt > std::chrono::seconds(5));
	}
	if (needRefresh) refreshComPorts();

	std::lock_guard<std::mutex> lk(g_state.portMtx);
	return {
		{"ports", g_state.cachedPorts},
		{"arduinoPort", g_state.cachedArduinoPort}
	};
}

static json handleCompile(const json& params) {
	Builder::CompileRequest req;
	req.sketchDir = params.value("sketchDir", "");
	req.workspaceDir = params.value("workspaceDir", "");
	req.fqbn = params.value("fqbn", "");
	req.forceFullBuild = params.value("forceFullBuild", false);

	if (req.sketchDir.empty()) throw std::runtime_error("sketchDir required");

	auto r = Builder::compile(req);
	return Builder::toJson(r);
}

static json handleUpload(const json& params) {
	Builder::UploadRequest req;
	req.sketchDir = params.value("sketchDir", "");
	req.workspaceDir = params.value("workspaceDir", "");
	req.port = params.value("port", "");
	req.skipCompile = params.value("skipCompile", false);

	if (req.sketchDir.empty()) throw std::runtime_error("sketchDir required");

	auto r = Builder::upload(req);
	return Builder::toJson(r);
}

static json handleInvalidateCache(const json& params) {
	std::string sketchDir = params.value("sketchDir", "");
	std::lock_guard<std::mutex> lk(g_state.sketchMtx);
	if (sketchDir.empty()) {
		g_state.sketches.clear();
	} else {
		g_state.sketches.erase(sketchDir);
	}
	return json::object();
}

// =============================================================================
// 1 接続を処理
// =============================================================================
static void processOneConnection(HANDLE hPipe) {
	std::string body = PipeIO::readMessage(hPipe);
	if (body.empty()) return;

	json req;
	json reqId = nullptr;
	try {
		req = json::parse(body);
	} catch (std::exception& e) {
		auto resp = buildErrorResponse(nullptr, -32700,
			std::string("Parse error: ") + e.what());
		PipeIO::writeMessage(hPipe, resp.dump());
		return;
	}

	reqId = req.value("id", json(nullptr));
	std::string method = req.value("method", "");
	json params = req.value("params", json::object());

	g_state.requestCount.fetch_add(1);
	g_state.lastRequestAt = std::chrono::steady_clock::now();

	logInfo("RPC: " + method);

	json resp;
	try {
		json result;
		if (method == "ping")                 result = handlePing();
		else if (method == "listPorts")       result = handleListPorts();
		else if (method == "compile")         result = handleCompile(params);
		else if (method == "upload")          result = handleUpload(params);
		else if (method == "invalidateCache") result = handleInvalidateCache(params);
		else if (method == "shutdown") {
			result = json::object();
			g_state.shutdownRequested = true;
		} else throw std::runtime_error("Unknown method: " + method);
		resp = buildOkResponse(reqId, result);
	} catch (std::exception& e) {
		logErr(std::string("RPC error: ") + e.what());
		resp = buildErrorResponse(reqId, -32603, e.what());
	}

	PipeIO::writeMessage(hPipe, resp.dump());
}

// =============================================================================
// パイプサーバ メインループ (シリアル処理: 同時 1 ジョブ)
// =============================================================================
static void runPipeServer() {
	const std::string pipeName = PipeIO::makePipeName();
	logInfo("Pipe server starting at " + pipeName);

	PSECURITY_DESCRIPTOR pSD = nullptr;
	SECURITY_ATTRIBUTES sa{};
	auto* psa = makePipeSecurity(sa, pSD);

	while (!g_state.shutdownRequested) {
		HANDLE hPipe = CreateNamedPipeA(
			pipeName.c_str(),
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT |
			PIPE_REJECT_REMOTE_CLIENTS,
			PIPE_UNLIMITED_INSTANCES,
			64 * 1024,
			64 * 1024,
			0,
			psa);

		if (hPipe == INVALID_HANDLE_VALUE) {
			logErr("CreateNamedPipe failed: " + std::to_string(GetLastError()));
			Sleep(500);
			continue;
		}

		// 接続待ち (shutdown 通知は「自分で接続して即切断」する)
		BOOL connected = ConnectNamedPipe(hPipe, nullptr) ?
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (g_state.shutdownRequested) {
			DisconnectNamedPipe(hPipe);
			CloseHandle(hPipe);
			break;
		}

		if (!connected) {
			CloseHandle(hPipe);
			continue;
		}

		try {
			processOneConnection(hPipe);
		} catch (std::exception& e) {
			logErr(std::string("Unhandled exception: ") + e.what());
		}

		FlushFileBuffers(hPipe);
		DisconnectNamedPipe(hPipe);
		CloseHandle(hPipe);
	}

	if (pSD) LocalFree(pSD);
	logInfo("Pipe server stopped.");
}

// =============================================================================
// アイドル監視: 30 分無リクエストで shutdown
// =============================================================================
static void idleWatchdog(int idleMinutes) {
	using namespace std::chrono;
	const auto timeout = minutes(idleMinutes);
	while (!g_state.shutdownRequested) {
		std::this_thread::sleep_for(seconds(60));
		auto idle = steady_clock::now() - g_state.lastRequestAt;
		if (idle > timeout) {
			logInfo("Idle timeout reached - shutting down.");
			g_state.shutdownRequested = true;
			// パイプサーバの ConnectNamedPipe を起こすため自分で 1 回 connect
			HANDLE h = CreateFileA(PipeIO::makePipeName().c_str(),
				GENERIC_READ | GENERIC_WRITE, 0, nullptr,
				OPEN_EXISTING, 0, nullptr);
			if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
			break;
		}
	}
}

// =============================================================================
// COM ポート変化監視: HKLM\HARDWARE\DEVICEMAP\SERIALCOMM
// =============================================================================
static void portChangeWatcher() {
	HKEY hKey = nullptr;
	LONG r = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0,
		KEY_NOTIFY | KEY_READ, &hKey);
	if (r != ERROR_SUCCESS) {
		logWarn("Cannot open SERIALCOMM for change notify");
		return;
	}
	HANDLE hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);

	while (!g_state.shutdownRequested) {
		ResetEvent(hEvent);
		LONG nr = RegNotifyChangeKeyValue(hKey, FALSE,
			REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
		if (nr != ERROR_SUCCESS) {
			Sleep(2000);
			continue;
		}
		// 5 秒タイムアウトで shutdown チェックを兼ねる
		DWORD wr = WaitForSingleObject(hEvent, 5000);
		if (g_state.shutdownRequested) break;
		if (wr == WAIT_OBJECT_0) {
			refreshComPorts();
			logInfo("COM port list refreshed (registry changed)");
		}
	}

	CloseHandle(hEvent);
	RegCloseKey(hKey);
}

// =============================================================================
// Ctrl-C ハンドラ (前景実行時のクリーン終了用)
// =============================================================================
static BOOL WINAPI consoleHandler(DWORD signal) {
	if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT
		|| signal == CTRL_CLOSE_EVENT) {
		logInfo("Console signal received, shutting down");
		g_state.shutdownRequested = true;
		HANDLE h = CreateFileA(PipeIO::makePipeName().c_str(),
			GENERIC_READ | GENERIC_WRITE, 0, nullptr,
			OPEN_EXISTING, 0, nullptr);
		if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
		return TRUE;
	}
	return FALSE;
}

// =============================================================================
// main
// =============================================================================
int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	// --- 引数 ---
	int idleMinutes = 30;
	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
		if (a == "--no-daemonize" || a == "--foreground") g_foreground = true;
		else if (a == "--daemonize") g_foreground = false;
		else if (a == "--idle-minutes" && i + 1 < argc) {
			idleMinutes = std::atoi(argv[++i]);
		}
	}

	initLogger();
	logInfo("=== arduino-build-daemon starting ===");

	// --- シングルインスタンス ---
	HANDLE hMutex = CreateMutexA(nullptr, FALSE, PipeIO::makeMutexName().c_str());
	if (hMutex == nullptr) {
		logErr("CreateMutex failed");
		return 1;
	}
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		logInfo("Another instance is already running. Exiting.");
		return 0;
	}

	// --- ツールチェーン解決 ---
	if (!initializeDaemonState()) {
		logErr("Toolchain init failed: " + g_state.toolchain.errorMessage);
		// 起動だけは継続。ping で error を返せるように。
	} else {
		logInfo("Toolchain OK: " + g_state.toolchain.compilerVersion);
	}

	SetConsoleCtrlHandler(consoleHandler, TRUE);

	// --- ワーカースレッド ---
	std::thread idleThread(idleWatchdog, idleMinutes);
	std::thread portThread(portChangeWatcher);

	// --- メインループ ---
	runPipeServer();

	g_state.shutdownRequested = true;
	if (idleThread.joinable()) idleThread.join();
	if (portThread.joinable()) portThread.join();

	logInfo("=== daemon exit ===");
	CloseHandle(hMutex);
	return 0;
}