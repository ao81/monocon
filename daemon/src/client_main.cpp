#include "pipe_io.h"
#include "utils.h"

#include <windows.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <filesystem>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// =============================================================================
// arduino-build-cli: 薄いクライアント
//
// 使い方:
//   arduino-build-cli.exe upload <SketchDir> [Port]
//   arduino-build-cli.exe build  <SketchDir>
//   arduino-build-cli.exe ping
//   arduino-build-cli.exe ports
//   arduino-build-cli.exe shutdown
// =============================================================================

namespace {

	// daemon exe の場所を決める。クライアントと同じフォルダに置く前提。
	std::string findDaemonExe() {
		char buf[MAX_PATH];
		DWORD r = GetModuleFileNameA(nullptr, buf, MAX_PATH);
		if (r == 0 || r == MAX_PATH) return "";
		std::filesystem::path p(buf);
		return (p.parent_path() / "arduino-build-daemon.exe").string();
	}

	// daemon を DETACHED で起動 (親終了に巻き込まれない)
	bool spawnDaemon(const std::string& daemonExe) {
		if (!Utils::fileExists(daemonExe)) {
			std::cerr << ">>> daemon executable not found: " << daemonExe << "\n";
			return false;
		}
		STARTUPINFOA si{};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi{};
		std::string cmd = "\"" + daemonExe + "\" --daemonize";

		BOOL ok = CreateProcessA(
			nullptr, cmd.data(), nullptr, nullptr, FALSE,
			DETACHED_PROCESS | CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB,
			nullptr, nullptr, &si, &pi);
		if (!ok) {
			std::cerr << ">>> CreateProcess failed: " << GetLastError() << "\n";
			return false;
		}
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}

	// パイプ接続: 失敗時は daemon を起こしてリトライ
	HANDLE connectOrSpawn(const std::string& pipeName, bool autoSpawn) {
		bool spawned = false;
		for (int attempt = 0; attempt < 60; ++attempt) {
			HANDLE h = CreateFileA(
				pipeName.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0, nullptr, OPEN_EXISTING, 0, nullptr);
			if (h != INVALID_HANDLE_VALUE) {
				DWORD mode = PIPE_READMODE_BYTE;
				SetNamedPipeHandleState(h, &mode, nullptr, nullptr);
				return h;
			}
			DWORD err = GetLastError();
			if (err == ERROR_PIPE_BUSY) {
				WaitNamedPipeA(pipeName.c_str(), 1000);
				continue;
			}
			// daemon が無い
			if (!spawned && autoSpawn) {
				spawned = spawnDaemon(findDaemonExe());
				if (!spawned) return INVALID_HANDLE_VALUE;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		return INVALID_HANDLE_VALUE;
	}

	// JSON-RPC リクエスト送信 (タイムアウト付き)
	std::string sendRpc(HANDLE hPipe, const std::string& method,
		const json& params, int timeoutSec = 120) {
		json req = {
			{"jsonrpc", "2.0"},
			{"id", 1},
			{"method", method},
			{"params", params}
		};
		std::string body = req.dump();

		if (!PipeIO::writeMessage(hPipe, body)) return "";

		// タイムアウト処理: 簡易的に readMessage を別スレッドにする
		std::string resp;
		std::atomic<bool> done{ false };
		std::thread t([&]() {
			resp = PipeIO::readMessage(hPipe);
			done = true;
			});
		auto deadline = std::chrono::steady_clock::now() +
			std::chrono::seconds(timeoutSec);
		while (!done && std::chrono::steady_clock::now() < deadline) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		if (!done) {
			// Pipe を閉じることで read を解除させる
			CancelIoEx(hPipe, nullptr);
			t.join();
			return "";
		}
		t.join();
		return resp;
	}

	void printUsage() {
		std::cout <<
			"Usage:\n"
			"  arduino-build-cli.exe upload <SketchDir> [Port] [--workspace <Dir>]\n"
			"  arduino-build-cli.exe build  <SketchDir>        [--workspace <Dir>]\n"
			"  arduino-build-cli.exe ping\n"
			"  arduino-build-cli.exe ports\n"
			"  arduino-build-cli.exe shutdown\n"
			"\n"
			"  --workspace <Dir>: ビルド出力先のワークスペースルートを指定\n"
			"     省略時は SketchDir から .vscode/ を持つフォルダを上に探索\n"
			"     ビルド出力先: <Workspace>/.vscode/build/<相対パス>/\n";
	}

	// --workspace <path> を抽出して、それ以外の位置引数だけ vector で返す
	std::vector<std::string> parsePositional(int argc, char* argv[],
		std::string& workspaceOut) {
		std::vector<std::string> positional;
		for (int i = 1; i < argc; ++i) {
			std::string a = argv[i];
			if (a == "--workspace" && i + 1 < argc) {
				workspaceOut = argv[++i];
			} else {
				positional.push_back(a);
			}
		}
		return positional;
	}

	int handleSubcommand(int argc, char* argv[]) {
		std::string workspaceDir;
		auto pos = parsePositional(argc, argv, workspaceDir);
		if (pos.empty()) { printUsage(); return 1; }
		std::string cmd = pos[0];

		HANDLE hPipe = connectOrSpawn(PipeIO::makePipeName(),
			/*autoSpawn=*/cmd != "shutdown");
		if (hPipe == INVALID_HANDLE_VALUE) {
			std::cerr << ">>> Cannot connect to daemon\n";
			return 1;
		}

		std::string resp;
		Stopwatch sw;

		if (cmd == "ping") {
			resp = sendRpc(hPipe, "ping", json::object(), 5);
		} else if (cmd == "ports") {
			resp = sendRpc(hPipe, "listPorts", json::object(), 5);
		} else if (cmd == "shutdown") {
			resp = sendRpc(hPipe, "shutdown", json::object(), 5);
		} else if (cmd == "build") {
			if (pos.size() < 2) { printUsage(); CloseHandle(hPipe); return 1; }
			json p = { {"sketchDir", pos[1]} };
			if (!workspaceDir.empty()) p["workspaceDir"] = workspaceDir;
			resp = sendRpc(hPipe, "compile", p, 300);
		} else if (cmd == "upload") {
			if (pos.size() < 2) { printUsage(); CloseHandle(hPipe); return 1; }
			json p = { {"sketchDir", pos[1]} };
			if (pos.size() >= 3) p["port"] = pos[2];
			if (!workspaceDir.empty()) p["workspaceDir"] = workspaceDir;
			resp = sendRpc(hPipe, "upload", p, 300);
		} else {
			printUsage();
			CloseHandle(hPipe);
			return 1;
		}

		CloseHandle(hPipe);

		if (resp.empty()) {
			std::cerr << ">>> No response from daemon (timeout?)\n";
			return 1;
		}

		// 結果を整形して表示
		try {
			json j = json::parse(resp);
			if (j.contains("error")) {
				std::cerr << ">>> Error: " << j["error"]["message"].get<std::string>() << "\n";
				return 1;
			}
			const auto& r = j["result"];
			if (cmd == "ping") {
				std::cout << "Daemon version: " << r.value("version", "?")
					<< " (uptime " << r.value("uptimeSec", 0) << "s, "
					<< r.value("requestCount", 0) << " requests)\n";
				std::cout << "Toolchain: " << r.value("compilerVersion", "?") << "\n";
				if (!r.value("toolchainValid", false)) {
					std::cerr << ">>> Toolchain error: "
						<< r.value("toolchainError", "?") << "\n";
				}
			} else if (cmd == "ports") {
				std::cout << "Ports:\n";
				for (const auto& p : r["ports"]) std::cout << "  " << p.get<std::string>() << "\n";
				std::cout << "Auto-detected Arduino: " << r.value("arduinoPort", "(none)") << "\n";
			} else if (cmd == "build") {
				if (!r.value("success", false)) {
					std::cerr << ">>> Compile failed: " << r.value("errorMessage", "") << "\n";
					std::cerr << r.value("compilerOutput", "") << "\n";
					return 1;
				}
				std::cout << "Compile " << (r.value("cached", false) ? "(cached)" : "OK")
					<< " in " << r.value("buildTimeMs", 0.0) << " ms\n";
				std::cout << "  hex: " << r.value("hexFile", "") << "\n";
			} else if (cmd == "upload") {
				if (!r.value("success", false)) {
					std::cerr << ">>> Upload failed: " << r.value("errorMessage", "") << "\n";
					std::cerr << r.value("avrdudeOutput", "") << "\n";
					if (r.contains("compile") && !r["compile"].value("success", true)) {
						std::cerr << r["compile"].value("compilerOutput", "") << "\n";
					}
					return 1;
				}
				const auto& cr = r["compile"];
				std::cout << "Compile " << (cr.value("cached", false) ? "(cached)" : "OK")
					<< " in " << cr.value("buildTimeMs", 0.0) << " ms\n";
				std::cout << "Upload to " << r.value("port", "") << " in "
					<< r.value("uploadTimeMs", 0.0) << " ms\n";
				std::cout << "Total client time: " << sw.elapsedMilliseconds() << " ms\n";
			} else if (cmd == "shutdown") {
				std::cout << "Daemon shutdown requested.\n";
			}
		} catch (std::exception& e) {
			std::cerr << ">>> Bad JSON response: " << e.what() << "\n";
			std::cerr << resp << "\n";
			return 1;
		}

		return 0;
	}

} // namespace

int main(int argc, char* argv[]) {
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	try {
		return handleSubcommand(argc, argv);
	} catch (std::exception& e) {
		std::cerr << ">>> Exception: " << e.what() << "\n";
		return 1;
	}
}