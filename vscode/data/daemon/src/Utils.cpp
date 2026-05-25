#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <wincrypt.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>

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
	// GetFileAttributesEx は CreateFile/CloseHandle を必要としないため、
	// ハンドル確保のオーバーヘッド (NTFS journaling 等) を回避できる。
	// 数百ファイルを舐めるホットパスで効く (NTFS 上で 2x-5x 高速)。
	long long getFileLastWriteTime(const std::string& path) {
		WIN32_FILE_ATTRIBUTE_DATA fad{};
		if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fad)) return 0;
		ULARGE_INTEGER uli;
		uli.LowPart = fad.ftLastWriteTime.dwLowDateTime;
		uli.HighPart = fad.ftLastWriteTime.dwHighDateTime;
		return static_cast<long long>(uli.QuadPart);
	}

	// 1 syscall で mtime + size を取得 (差分判定の syscall を半減)
	bool getFileMetadata(const std::string& path, long long& mtime, uint64_t& size) {
		WIN32_FILE_ATTRIBUTE_DATA fad{};
		if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fad)) {
			mtime = 0;
			size = 0;
			return false;
		}
		ULARGE_INTEGER uli;
		uli.LowPart = fad.ftLastWriteTime.dwLowDateTime;
		uli.HighPart = fad.ftLastWriteTime.dwHighDateTime;
		mtime = static_cast<long long>(uli.QuadPart);
		ULARGE_INTEGER sz;
		sz.LowPart = fad.nFileSizeLow;
		sz.HighPart = fad.nFileSizeHigh;
		size = static_cast<uint64_t>(sz.QuadPart);
		return true;
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
// runProcess
//
// 旧実装は 5ms 間隔のポーリングで stdout/stderr を drain しつつ
// プロセス終了を待っていたため、子プロセス 1 つあたり最悪 +5ms の遅延と
// 反復シスコールコストを払っていた。
//
// 新実装はパイプを 2 本ともブロッキング ReadFile するスレッドに分担し、
// メインスレッドは WaitForSingleObject(INFINITE) でプロセス終了だけを待つ。
// 子が書き込み端を閉じれば ReadFile は EOF (bytesRead=0) で戻り、スレッドは
// 自然終了する。
//   - ポーリング遅延ゼロ
//   - WaitForSingleObject の最小 quantum (1ms) より細かい単位で起き直す
//   - WriteFile/ReadFile 内部バッファリングを最大限活用
// =============================================================================
namespace {
	void drainPipeBlocking(HANDLE hPipe, std::string& sink, bool capture) {
		// 64KB のブロックで吸い上げる。avr-gcc の警告/エラーでも数 KB 程度。
		char buffer[64 * 1024];
		DWORD bytesRead = 0;
		while (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, nullptr)
			&& bytesRead > 0) {
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

	// バッファ 0 指定はカーネル既定 (~4KB)。子が大量出力するとブロッキング
	// 書込で詰まるが、こちらが drain スレッドで常時吸い上げるため問題なし。
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

	// 親側の書込端は閉じる。子側だけが書込端を持つ状態にしておかないと、
	// 子が exit しても ReadFile が EOF を返さずスレッドが終わらない。
	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);

	if (!ok) {
		CloseHandle(hStdOutRead);
		CloseHandle(hStdErrRead);
		return result;
	}

	// stderr は同期スレッドで、stdout は当スレッドで読む (スレッド 1 個で済む)。
	// 出力が多い側を別スレッドに回せばどちらでも良いが、コンパイラ診断は
	// 主に stderr に出るので stderr 側を別スレッドにする方が体感が早い。
	std::thread errThread([&] {
		drainPipeBlocking(hStdErrRead, result.error, captureOutput);
	});
	drainPipeBlocking(hStdOutRead, result.output, captureOutput);
	errThread.join();

	// 両パイプから EOF が来た時点で子は exit 済み or もう書込しない。
	// WaitForSingleObject は実質ノーウェイトで返る。
	WaitForSingleObject(pi.hProcess, INFINITE);

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