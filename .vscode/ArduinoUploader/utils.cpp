#include "utils.h"

#include <windows.h>
#include <shlobj.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

// =============================================================================
// Utils 実装
// =============================================================================
namespace Utils {

	// ----- パス取得 -----
	std::string getLocalAppDataPath() {
		char path[MAX_PATH];
		if (SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path) == S_OK) {
			return std::string(path);
		}
		return "";
	}

	std::string getGlobalCacheDir() {
		return joinPath(getLocalAppDataPath(), "ArduinoCLI_Cache");
	}

	// ----- 存在チェック -----
	bool fileExists(const std::string& path) {
		std::error_code ec;
		return fs::is_regular_file(path, ec);
	}

	bool directoryExists(const std::string& path) {
		std::error_code ec;
		return fs::is_directory(path, ec);
	}

	// ----- ファイル I/O -----
	std::string readFile(const std::string& path) {
		std::ifstream file(path, std::ios::binary);
		if (!file) {
			throw std::runtime_error("Cannot read file: " + path);
		}
		std::ostringstream ss;
		ss << file.rdbuf();
		return ss.str();
	}

	std::vector<std::string> readLines(const std::string& path) {
		std::vector<std::string> lines;
		std::ifstream file(path, std::ios::binary);
		if (!file) return lines;

		std::string line;
		while (std::getline(file, line)) {
			// 末尾の \r を除去 (CRLF対応)
			if (!line.empty() && line.back() == '\r') line.pop_back();
			lines.push_back(std::move(line));
		}
		return lines;
	}

	void writeFile(const std::string& path, const std::string& content) {
		std::ofstream file(path, std::ios::binary);
		if (!file) {
			throw std::runtime_error("Cannot write file: " + path);
		}
		file.write(content.data(), static_cast<std::streamsize>(content.size()));
	}

	void writeLines(const std::string& path, const std::vector<std::string>& lines) {
		// 1回の書き込みにまとめる(I/O 回数削減)
		std::string buffer;
		size_t total = 0;
		for (const auto& l : lines) total += l.size() + 1;
		buffer.reserve(total);
		for (const auto& l : lines) {
			buffer.append(l);
			buffer.push_back('\n');
		}
		writeFile(path, buffer);
	}

	bool writeFileIfChanged(const std::string& path, const std::string& content) {
		if (fileExists(path)) {
			try {
				if (readFile(path) == content) {
					return false; // 変更なし
				}
			} catch (...) {
				// 読み込み失敗時はそのまま書き込み
			}
		}
		writeFile(path, content);
		return true;
	}

	// ----- ディレクトリ操作 -----
	void createDirectory(const std::string& path) {
		std::error_code ec;
		fs::create_directories(path, ec);
	}

	void deleteFile(const std::string& path) {
		std::error_code ec;
		fs::remove(path, ec);
	}

	void cleanDirectory(const std::string& dir, const std::string& extension) {
		if (!directoryExists(dir)) return;

		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(dir, ec)) {
			if (ec) break;
			if (entry.is_regular_file(ec) && entry.path().extension().string() == extension) {
				std::error_code rmEc;
				fs::remove(entry.path(), rmEc);
			}
		}
	}

	// ----- パス操作 -----
	std::string joinPath(const std::string& base, const std::string& relative) {
		return (fs::path(base) / relative).string();
	}

	std::string getFileName(const std::string& path) {
		return fs::path(path).filename().string();
	}

	std::string getFileStem(const std::string& path) {
		return fs::path(path).stem().string();
	}

	std::string getParentDirectory(const std::string& path) {
		return fs::path(path).parent_path().string();
	}

	// ----- ファイル列挙 -----
	std::vector<std::string> getFilesByExtension(const std::string& dir, const std::string& extension) {
		std::vector<std::string> files;
		if (!directoryExists(dir)) return files;

		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(dir, ec)) {
			if (ec) break;
			if (entry.is_regular_file(ec) && entry.path().extension().string() == extension) {
				files.push_back(entry.path().string());
			}
		}
		// 順序を安定化(ハッシュの一貫性のため)
		std::sort(files.begin(), files.end());
		return files;
	}

	std::vector<std::string> getDirectories(const std::string& dir) {
		std::vector<std::string> dirs;
		if (!directoryExists(dir)) return dirs;

		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(dir, ec)) {
			if (ec) break;
			if (entry.is_directory(ec)) {
				dirs.push_back(entry.path().string());
			}
		}
		return dirs;
	}

	std::string getLatestDirectory(const std::string& baseDir) {
		auto dirs = getDirectories(baseDir);
		if (dirs.empty()) return "";

		std::string latest = dirs[0];
		long long latestTime = getFileLastWriteTime(latest);

		for (size_t i = 1; i < dirs.size(); ++i) {
			long long t = getFileLastWriteTime(dirs[i]);
			if (t > latestTime) {
				latestTime = t;
				latest = dirs[i];
			}
		}
		return latest;
	}

	// ----- ハッシュ/タイムスタンプ -----
	long long getFileLastWriteTime(const std::string& path) {
		std::error_code ec;
		auto ftime = fs::last_write_time(path, ec);
		if (ec) return 0;

		// file_clock -> system_clock 変換
		auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
		return std::chrono::duration_cast<std::chrono::milliseconds>(sctp.time_since_epoch()).count();
	}

	std::string getSourceSignature(const std::vector<std::string>& files) {
		std::ostringstream oss;
		for (size_t i = 0; i < files.size(); ++i) {
			if (i > 0) oss << ';';
			// ファイル名を含めることで「ファイル追加/削除」も検出
			oss << getFileName(files[i]) << ':' << getFileLastWriteTime(files[i]);
		}
		return oss.str();
	}

	// ----- 文字列操作 -----
	std::string trim(const std::string& str) {
		const char* whitespace = " \t\n\r";
		size_t first = str.find_first_not_of(whitespace);
		if (first == std::string::npos) return "";
		size_t last = str.find_last_not_of(whitespace);
		return str.substr(first, last - first + 1);
	}

	std::vector<std::string> split(const std::string& str, char delimiter) {
		std::vector<std::string> tokens;
		std::stringstream ss(str);
		std::string token;
		while (std::getline(ss, token, delimiter)) {
			tokens.push_back(std::move(token));
		}
		return tokens;
	}

	std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
		if (from.empty()) return str;
		size_t pos = 0;
		while ((pos = str.find(from, pos)) != std::string::npos) {
			str.replace(pos, from.length(), to);
			pos += to.length();
		}
		return str;
	}

} // namespace Utils

// =============================================================================
// Stopwatch
// =============================================================================
Stopwatch::Stopwatch() {
	start();
}

void Stopwatch::start() {
	start_time_ = std::chrono::steady_clock::now();
	running_ = true;
}

void Stopwatch::stop() {
	end_time_ = std::chrono::steady_clock::now();
	running_ = false;
}

void Stopwatch::restart() {
	start();
}

double Stopwatch::elapsedMilliseconds() const {
	auto end = running_ ? std::chrono::steady_clock::now() : end_time_;
	return std::chrono::duration<double, std::milli>(end - start_time_).count();
}

double Stopwatch::elapsedSeconds() const {
	return elapsedMilliseconds() / 1000.0;
}

// =============================================================================
// runProcess: stdout/stderr を非ブロッキングに読みながら子プロセスを待つ
//
// 旧実装は WaitForSingleObject(INFINITE) で先に子プロセスの終了を待ち、
// その後でパイプを読んでいた。子プロセスの出力がパイプバッファ(~4KB)を
// 超えると子プロセスが書き込み待ちでブロックし、フルコンパイル時に
// 数百KB〜数MB の出力を吐く arduino-cli では事実上ハング/極端な遅延が
// 発生していた。これが「キャッシュ保存(=フルビルド)が遅い」主因。
//
// 改善版:
//   - PeekNamedPipe + ReadFile を一定間隔でポーリング
//   - パイプバッファをこまめに空にするので子プロセスはブロックしない
//   - captureOutput=false なら出力読み込みコスト自体を削減できる
// =============================================================================
namespace {

	// 利用可能なバイトをすべて読み出す(ノンブロッキング)
	void drainPipe(HANDLE hPipe, std::string& sink, bool capture) {
		DWORD available = 0;
		if (!PeekNamedPipe(hPipe, nullptr, 0, nullptr, &available, nullptr)) return;
		if (available == 0) return;

		char buffer[8192];
		DWORD bytesRead = 0;
		DWORD toRead = available > sizeof(buffer) ? sizeof(buffer) : available;
		if (ReadFile(hPipe, buffer, toRead, &bytesRead, nullptr) && bytesRead > 0) {
			if (capture) {
				sink.append(buffer, bytesRead);
			}
		}
	}

	// パイプに残っているものを最後まで吸い出す(プロセス終了後)
	void drainPipeUntilEof(HANDLE hPipe, std::string& sink, bool capture) {
		char buffer[8192];
		DWORD bytesRead = 0;
		while (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
			if (capture) {
				sink.append(buffer, bytesRead);
			}
		}
	}

} // namespace

ProcessResult runProcess(const std::string& command,
	const std::string& args,
	const std::string& workingDir,
	bool captureOutput) {
	ProcessResult result;
	result.exitCode = -1;

	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = nullptr;

	HANDLE hStdOutRead = nullptr, hStdOutWrite = nullptr;
	HANDLE hStdErrRead = nullptr, hStdErrWrite = nullptr;

	if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) {
		return result;
	}
	if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0)) {
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		return result;
	}

	// 親側のハンドルは継承しない
	SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si{};
	si.cb = sizeof(si);
	si.hStdOutput = hStdOutWrite;
	si.hStdError = hStdErrWrite;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.dwFlags = STARTF_USESTDHANDLES;

	PROCESS_INFORMATION pi{};
	std::string cmdLine = "\"" + command + "\" " + args;

	BOOL ok = CreateProcessA(
		nullptr,
		cmdLine.data(),
		nullptr,
		nullptr,
		TRUE,
		CREATE_NO_WINDOW,
		nullptr,
		workingDir.empty() ? nullptr : workingDir.c_str(),
		&si,
		&pi);

	// 子プロセスへ渡した書き込みハンドルは親では即閉じる
	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);

	if (!ok) {
		CloseHandle(hStdOutRead);
		CloseHandle(hStdErrRead);
		return result;
	}

	// 子プロセスの出力を非ブロッキングに読みながら終了を待つ
	constexpr DWORD kPollIntervalMs = 20;
	while (true) {
		drainPipe(hStdOutRead, result.output, captureOutput);
		drainPipe(hStdErrRead, result.error, captureOutput);

		DWORD waitResult = WaitForSingleObject(pi.hProcess, kPollIntervalMs);
		if (waitResult == WAIT_OBJECT_0) {
			break;  // 子プロセス終了
		}
		// タイムアウトならループ継続
	}

	// 終了後、パイプに残っているものを最後まで吸い出す
	drainPipeUntilEof(hStdOutRead, result.output, captureOutput);
	drainPipeUntilEof(hStdErrRead, result.error, captureOutput);

	DWORD exitCode = 0;
	if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
		result.exitCode = static_cast<int>(exitCode);
	}

	CloseHandle(hStdOutRead);
	CloseHandle(hStdErrRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return result;
}