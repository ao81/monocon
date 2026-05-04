#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <wincrypt.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "advapi32.lib")

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

	// 自プロセス (arduino-build-daemon.exe / arduino-build-cli.exe) の
	// 実行ファイルが置かれているディレクトリを返す。
	// 例: C:\VSCode\data\daemon\build\bin
	std::string getExecutableDir() {
		char path[MAX_PATH];
		DWORD len = GetModuleFileNameA(nullptr, path, MAX_PATH);
		if (len == 0 || len == MAX_PATH) return "";
		return fs::path(path).parent_path().string();
	}

	// VSCode のポータブル data ディレクトリ配下の cache/ を返す。
	//
	// 想定する配置:
	//   <VSCode>/
	//     Code.exe
	//     data/
	//       daemon/build/bin/arduino-build-{cli,daemon}.exe   <- exe はここ
	//       cache/                                            <- これを返す
	//
	// exe からの相対 "..\..\..\cache" で data/cache/ に到達する。
	// 解決に失敗した場合は %LOCALAPPDATA%\ArduinoBuildDaemon\cache を使う。
	std::string getDataCacheDir() {
		std::string exeDir = getExecutableDir();
		if (!exeDir.empty()) {
			std::error_code ec;
			fs::path candidate =
				fs::path(exeDir) / ".." / ".." / ".." / "cache";
			fs::path normalized = fs::weakly_canonical(candidate, ec);
			if (ec) normalized = candidate.lexically_normal();
			return normalized.string();
		}
		// フォールバック: 旧来の %LOCALAPPDATA% 配下
		return joinPath(getLocalAppDataPath(), "ArduinoBuildDaemon\\cache");
	}

	std::string getGlobalCacheDir() {
		return joinPath(getLocalAppDataPath(), "ArduinoBuildDaemon");
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
		if (!file) throw std::runtime_error("Cannot read file: " + path);
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
			if (!line.empty() && line.back() == '\r') line.pop_back();
			lines.push_back(std::move(line));
		}
		return lines;
	}

	void writeFile(const std::string& path, const std::string& content) {
		std::ofstream file(path, std::ios::binary);
		if (!file) throw std::runtime_error("Cannot write file: " + path);
		file.write(content.data(), static_cast<std::streamsize>(content.size()));
	}

	void writeLines(const std::string& path, const std::vector<std::string>& lines) {
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
				if (readFile(path) == content) return false;
			} catch (...) {}
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
		std::sort(files.begin(), files.end());
		return files;
	}

	std::vector<std::string> getDirectories(const std::string& dir) {
		std::vector<std::string> dirs;
		if (!directoryExists(dir)) return dirs;
		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(dir, ec)) {
			if (ec) break;
			if (entry.is_directory(ec)) dirs.push_back(entry.path().string());
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

	// ----- ハッシュ / タイムスタンプ -----
	long long getFileLastWriteTime(const std::string& path) {
		HANDLE h = CreateFileA(
			path.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS,
			nullptr);
		if (h == INVALID_HANDLE_VALUE) return 0;

		FILETIME ft{};
		BOOL ok = GetFileTime(h, nullptr, nullptr, &ft);
		CloseHandle(h);
		if (!ok) return 0;

		ULARGE_INTEGER uli;
		uli.LowPart = ft.dwLowDateTime;
		uli.HighPart = ft.dwHighDateTime;
		return static_cast<long long>(uli.QuadPart);
	}

	std::string getSourceSignature(const std::vector<std::string>& files) {
		std::ostringstream oss;
		for (size_t i = 0; i < files.size(); ++i) {
			if (i > 0) oss << ';';
			oss << getFileName(files[i]) << ':' << getFileLastWriteTime(files[i]);
		}
		return oss.str();
	}

	// ----- 文字列 -----
	std::string trim(const std::string& str) {
		const char* ws = " \t\n\r";
		size_t first = str.find_first_not_of(ws);
		if (first == std::string::npos) return "";
		size_t last = str.find_last_not_of(ws);
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

	// ----- SHA-1 (CryptoAPI) -----
	std::string sha1Hex(const std::string& data) {
		HCRYPTPROV hProv = 0;
		HCRYPTHASH hHash = 0;
		std::string result;

		if (!CryptAcquireContextA(&hProv, nullptr, nullptr, PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT)) return result;
		if (!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
			CryptReleaseContext(hProv, 0);
			return result;
		}
		if (CryptHashData(hHash, reinterpret_cast<const BYTE*>(data.data()),
			static_cast<DWORD>(data.size()), 0)) {
			BYTE buf[20];
			DWORD len = sizeof(buf);
			if (CryptGetHashParam(hHash, HP_HASHVAL, buf, &len, 0)) {
				std::ostringstream oss;
				oss << std::hex << std::setfill('0');
				for (DWORD i = 0; i < len; ++i) oss << std::setw(2) << (int)buf[i];
				result = oss.str();
			}
		}
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return result;
	}

} // namespace Utils

// =============================================================================
// Stopwatch
// =============================================================================
Stopwatch::Stopwatch() { start(); }
void Stopwatch::start() { start_time_ = std::chrono::steady_clock::now(); running_ = true; }
void Stopwatch::stop() { end_time_ = std::chrono::steady_clock::now(); running_ = false; }
void Stopwatch::restart() { start(); }

double Stopwatch::elapsedMilliseconds() const {
	auto end = running_ ? std::chrono::steady_clock::now() : end_time_;
	return std::chrono::duration<double, std::milli>(end - start_time_).count();
}

double Stopwatch::elapsedSeconds() const {
	return elapsedMilliseconds() / 1000.0;
}

// =============================================================================
// runProcess (utils.cpp 既存実装そのまま)
// =============================================================================
namespace {
	void drainPipe(HANDLE hPipe, std::string& sink, bool capture) {
		DWORD available = 0;
		if (!PeekNamedPipe(hPipe, nullptr, 0, nullptr, &available, nullptr)) return;
		if (available == 0) return;
		char buffer[8192];
		DWORD bytesRead = 0;
		DWORD toRead = available > sizeof(buffer) ? sizeof(buffer) : available;
		if (ReadFile(hPipe, buffer, toRead, &bytesRead, nullptr) && bytesRead > 0) {
			if (capture) sink.append(buffer, bytesRead);
		}
	}

	void drainPipeUntilEof(HANDLE hPipe, std::string& sink, bool capture) {
		char buffer[8192];
		DWORD bytesRead = 0;
		while (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0) {
			if (capture) sink.append(buffer, bytesRead);
		}
	}
}

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

	if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) return result;
	if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &sa, 0)) {
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		return result;
	}

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
		nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
		CREATE_NO_WINDOW, nullptr,
		workingDir.empty() ? nullptr : workingDir.c_str(),
		&si, &pi);

	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);

	if (!ok) {
		CloseHandle(hStdOutRead);
		CloseHandle(hStdErrRead);
		return result;
	}

	constexpr DWORD kPollIntervalMs = 5;
	while (true) {
		drainPipe(hStdOutRead, result.output, captureOutput);
		drainPipe(hStdErrRead, result.error, captureOutput);
		DWORD waitResult = WaitForSingleObject(pi.hProcess, kPollIntervalMs);
		if (waitResult == WAIT_OBJECT_0) break;
	}

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