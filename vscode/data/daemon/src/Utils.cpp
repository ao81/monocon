#include "utils.h"

#include <windows.h>
#include <shlobj.h>
#include <wincrypt.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iterator>
#include <thread>

#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

namespace {
	size_t validUtf8SequenceLength(const std::string& value, size_t offset) {
		const auto byte = static_cast<unsigned char>(value[offset]);
		size_t length = 0;
		if (byte >= 0xC2 && byte <= 0xDF) length = 2;
		else if (byte >= 0xE0 && byte <= 0xEF) length = 3;
		else if (byte >= 0xF0 && byte <= 0xF4) length = 4;
		else return 0;
		if (offset + length > value.size()) return 0;
		for (size_t i = 1; i < length; ++i) {
			const auto continuation =
				static_cast<unsigned char>(value[offset + i]);
			if ((continuation & 0xC0) != 0x80) return 0;
		}
		const auto second = static_cast<unsigned char>(value[offset + 1]);
		if (byte == 0xE0 && second < 0xA0) return 0;
		if (byte == 0xED && second >= 0xA0) return 0;
		if (byte == 0xF0 && second < 0x90) return 0;
		if (byte == 0xF4 && second >= 0x90) return 0;
		return length;
	}
}

// =============================================================================
// Utils 実装
// =============================================================================
namespace Utils {

	// ----- UTF-8 / Windows パス変換 -----
	std::wstring utf8ToWide(const std::string& value) {
		if (value.empty()) return {};
		int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			value.data(), static_cast<int>(value.size()), nullptr, 0);
		if (size <= 0) {
			throw std::runtime_error("Invalid UTF-8 path");
		}
		std::wstring result(static_cast<size_t>(size), L'\0');
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			value.data(), static_cast<int>(value.size()),
			result.data(), size) != size) {
			throw std::runtime_error("Cannot convert UTF-8 path");
		}
		return result;
	}

	std::string wideToUtf8(const std::wstring& value) {
		if (value.empty()) return {};
		int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			value.data(), static_cast<int>(value.size()), nullptr, 0,
			nullptr, nullptr);
		if (size <= 0) {
			throw std::runtime_error("Cannot encode Windows path as UTF-8");
		}
		std::string result(static_cast<size_t>(size), '\0');
		if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			value.data(), static_cast<int>(value.size()),
			result.data(), size, nullptr, nullptr) != size) {
			throw std::runtime_error("Cannot encode Windows path as UTF-8");
		}
		return result;
	}

	std::string normalizeToUtf8(const std::string& value) {
		std::string result;
		result.reserve(value.size());
		for (size_t i = 0; i < value.size();) {
			const auto byte = static_cast<unsigned char>(value[i]);
			if (byte < 0x80) {
				result.push_back(value[i++]);
				continue;
			}
			const size_t utf8Length = validUtf8SequenceLength(value, i);
			if (utf8Length > 0) {
				result.append(value, i, utf8Length);
				i += utf8Length;
				continue;
			}

			int byteCount = 1;
			if (IsDBCSLeadByteEx(CP_ACP, byte) && i + 1 < value.size()) {
				byteCount = 2;
			}
			wchar_t decoded[2]{};
			int wideCount = MultiByteToWideChar(CP_ACP, 0,
				value.data() + i, byteCount, decoded,
				static_cast<int>(std::size(decoded)));
			if (wideCount > 0) {
				result += wideToUtf8(
					std::wstring(decoded, static_cast<size_t>(wideCount)));
			}
			else {
				result += "\xEF\xBF\xBD";
			}
			i += static_cast<size_t>(byteCount);
		}
		return result;
	}

	fs::path pathFromUtf8(const std::string& path) {
		return fs::path(utf8ToWide(path));
	}

	std::string pathToUtf8(const fs::path& path) {
		return wideToUtf8(path.native());
	}

	std::string getShortPath(const std::string& path) {
		if (path.empty()) return path;
		const std::wstring widePath = utf8ToWide(path);
		const DWORD required = GetShortPathNameW(
			widePath.c_str(), nullptr, 0);
		if (required == 0) return path;
		std::wstring shortPath(static_cast<size_t>(required), L'\0');
		const DWORD written = GetShortPathNameW(
			widePath.c_str(), shortPath.data(), required);
		if (written == 0 || written >= required) return path;
		shortPath.resize(written);
		return wideToUtf8(shortPath);
	}

	// ----- パス取得 -----
	std::string getLocalAppDataPath() {
		wchar_t path[MAX_PATH];
		if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path) == S_OK) {
			return wideToUtf8(path);
		}
		return "";
	}

	std::string getGlobalCacheDir() {
		return joinPath(getLocalAppDataPath(), "ArduinoBuildDaemon");
	}

	// ----- 存在チェック -----
	bool fileExists(const std::string& path) {
		std::error_code ec;
		return fs::is_regular_file(pathFromUtf8(path), ec);
	}

	bool directoryExists(const std::string& path) {
		std::error_code ec;
		return fs::is_directory(pathFromUtf8(path), ec);
	}

	// ----- ファイル I/O -----
	std::string readFile(const std::string& path) {
		std::ifstream file(pathFromUtf8(path), std::ios::binary);
		if (!file) throw std::runtime_error("Cannot read file: " + path);
		std::ostringstream ss;
		ss << file.rdbuf();
		if (file.bad()) throw std::runtime_error("Cannot finish reading file: " + path);
		return ss.str();
	}

	std::string readFileLimited(const std::string& path, size_t maxBytes) {
		std::ifstream file(pathFromUtf8(path), std::ios::binary);
		if (!file) throw std::runtime_error("Cannot read file: " + path);
		std::string result;
		result.reserve(std::min<size_t>(maxBytes, 64 * 1024));
		std::vector<char> buffer(64 * 1024);
		while (file) {
			file.read(buffer.data(),
				static_cast<std::streamsize>(buffer.size()));
			const std::streamsize count = file.gcount();
			if (count <= 0) continue;
			const size_t bytes = static_cast<size_t>(count);
			if (bytes > maxBytes - result.size()) {
				throw std::runtime_error("File exceeds read limit: " + path);
			}
			result.append(buffer.data(), bytes);
		}
		if (file.bad()) {
			throw std::runtime_error("Cannot finish reading file: " + path);
		}
		return result;
	}

	std::vector<std::string> readLines(
		const std::string& path, size_t maxBytes) {
		std::vector<std::string> lines;
		std::string content;
		try {
			content = readFileLimited(path, maxBytes);
		}
		catch (...) {
			return lines;
		}
		std::istringstream file(content);
		std::string line;
		while (std::getline(file, line)) {
			if (!line.empty() && line.back() == '\r') line.pop_back();
			lines.push_back(std::move(line));
		}
		return lines;
	}

	void writeFile(const std::string& path, const std::string& content) {
		std::ofstream file(pathFromUtf8(path), std::ios::binary);
		if (!file) throw std::runtime_error("Cannot write file: " + path);
		file.write(content.data(), static_cast<std::streamsize>(content.size()));
		file.flush();
		if (!file) throw std::runtime_error("Cannot finish writing file: " + path);
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
				long long mtime = 0;
				uint64_t size = 0;
				if (getFileMetadata(path, mtime, size)
					&& size == content.size()
					&& readFileLimited(path, content.size()) == content) {
					return false;
				}
			} catch (...) {}
		}
		writeFile(path, content);
		return true;
	}

	// ----- ディレクトリ操作 -----
	void createDirectory(const std::string& path) {
		std::error_code ec;
		fs::create_directories(pathFromUtf8(path), ec);
		if (ec) throw std::runtime_error("Cannot create directory: " + path
			+ " (" + ec.message() + ")");
	}

	void deleteFile(const std::string& path) {
		std::error_code ec;
		fs::remove(pathFromUtf8(path), ec);
	}

	void cleanDirectory(const std::string& dir, const std::string& extension) {
		if (!directoryExists(dir)) return;
		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(pathFromUtf8(dir), ec)) {
			if (ec) break;
			if (entry.is_regular_file(ec)
				&& pathToUtf8(entry.path().extension()) == extension) {
				std::error_code rmEc;
				fs::remove(entry.path(), rmEc);
			}
		}
	}

	// ----- パス操作 -----
	std::string joinPath(const std::string& base, const std::string& relative) {
		return pathToUtf8(pathFromUtf8(base) / pathFromUtf8(relative));
	}

	std::string getFileName(const std::string& path) {
		return pathToUtf8(pathFromUtf8(path).filename());
	}

	std::string getFileStem(const std::string& path) {
		return pathToUtf8(pathFromUtf8(path).stem());
	}

	std::string getParentDirectory(const std::string& path) {
		return pathToUtf8(pathFromUtf8(path).parent_path());
	}

	// ----- ファイル列挙 -----
	std::vector<std::string> getFilesByExtension(const std::string& dir, const std::string& extension) {
		std::vector<std::string> files;
		if (!directoryExists(dir)) return files;
		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(pathFromUtf8(dir), ec)) {
			if (ec) break;
			if (entry.is_regular_file(ec)
				&& pathToUtf8(entry.path().extension()) == extension) {
				files.push_back(pathToUtf8(entry.path()));
			}
		}
		std::sort(files.begin(), files.end());
		return files;
	}

	std::vector<std::string> getDirectories(const std::string& dir) {
		std::vector<std::string> dirs;
		if (!directoryExists(dir)) return dirs;
		std::error_code ec;
		for (const auto& entry : fs::directory_iterator(pathFromUtf8(dir), ec)) {
			if (ec) break;
			if (entry.is_directory(ec)) dirs.push_back(pathToUtf8(entry.path()));
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
		std::wstring widePath = utf8ToWide(path);
		if (!GetFileAttributesExW(widePath.c_str(), GetFileExInfoStandard, &fad)) return 0;
		ULARGE_INTEGER uli;
		uli.LowPart = fad.ftLastWriteTime.dwLowDateTime;
		uli.HighPart = fad.ftLastWriteTime.dwHighDateTime;
		return static_cast<long long>(uli.QuadPart);
	}

	// 1 syscall で mtime + size を取得 (差分判定の syscall を半減)
	bool getFileMetadata(const std::string& path, long long& mtime, uint64_t& size) {
		WIN32_FILE_ATTRIBUTE_DATA fad{};
		std::wstring widePath = utf8ToWide(path);
		if (!GetFileAttributesExW(widePath.c_str(), GetFileExInfoStandard, &fad)) {
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
	namespace {
		std::string finishSha1(HCRYPTHASH hash) {
			BYTE bytes[20];
			DWORD length = sizeof(bytes);
			if (!CryptGetHashParam(hash, HP_HASHVAL, bytes, &length, 0)) {
				return "";
			}
			std::ostringstream output;
			output << std::hex << std::setfill('0');
			for (DWORD i = 0; i < length; ++i) {
				output << std::setw(2) << static_cast<int>(bytes[i]);
			}
			return output.str();
		}

		bool hashBytes(HCRYPTHASH hash, const char* data, size_t size) {
			constexpr size_t HASH_CHUNK_SIZE = 1024 * 1024;
			for (size_t offset = 0; offset < size;) {
				const size_t chunk = std::min(HASH_CHUNK_SIZE, size - offset);
				if (!CryptHashData(hash,
					reinterpret_cast<const BYTE*>(data + offset),
					static_cast<DWORD>(chunk), 0)) {
					return false;
				}
				offset += chunk;
			}
			return true;
		}
	}

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
		if (hashBytes(hHash, data.data(), data.size())) {
			result = finishSha1(hHash);
		}
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return result;
	}

	std::string sha1FileHex(const std::string& path) {
		std::ifstream file(pathFromUtf8(path), std::ios::binary);
		if (!file) return "";

		HCRYPTPROV provider = 0;
		HCRYPTHASH hash = 0;
		if (!CryptAcquireContextA(&provider, nullptr, nullptr, PROV_RSA_FULL,
			CRYPT_VERIFYCONTEXT)) {
			return "";
		}
		if (!CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash)) {
			CryptReleaseContext(provider, 0);
			return "";
		}

		std::string result;
		std::vector<char> buffer(1024 * 1024);
		bool ok = true;
		while (file) {
			file.read(buffer.data(),
				static_cast<std::streamsize>(buffer.size()));
			const std::streamsize count = file.gcount();
			if (count > 0 && !hashBytes(
				hash, buffer.data(), static_cast<size_t>(count))) {
				ok = false;
				break;
			}
		}
		if (file.bad()) ok = false;
		if (ok) result = finishSha1(hash);

		CryptDestroyHash(hash);
		CryptReleaseContext(provider, 0);
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
	constexpr size_t MAX_CAPTURED_PROCESS_OUTPUT = 16 * 1024 * 1024;

	void drainPipeBlocking(HANDLE hPipe, std::string& sink, bool capture,
		bool& truncated) {
		// 64KB のブロックで吸い上げる。avr-gcc の警告/エラーでも数 KB 程度。
		std::vector<char> buffer(64 * 1024);
		DWORD bytesRead = 0;
		while (ReadFile(hPipe, buffer.data(),
			static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)
			&& bytesRead > 0) {
			if (!capture) continue;
			const size_t remaining = sink.size() < MAX_CAPTURED_PROCESS_OUTPUT
				? MAX_CAPTURED_PROCESS_OUTPUT - sink.size() : 0;
			const size_t appendBytes =
				std::min<size_t>(remaining, bytesRead);
			if (appendBytes > 0) sink.append(buffer.data(), appendBytes);
			if (appendBytes < bytesRead) truncated = true;
		}
	}
}

ProcessResult runProcess(const std::string& command,
	const std::string& args,
	const std::string& workingDir,
	bool captureOutput,
	unsigned long timeoutMs) {
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
	HANDLE hStdIn = CreateFileW(
		L"NUL", GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		&sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hStdIn == INVALID_HANDLE_VALUE) {
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		CloseHandle(hStdErrRead);
		CloseHandle(hStdErrWrite);
		if (captureOutput) result.error = "Cannot open NUL for child stdin";
		return result;
	}

	SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOEXW si{};
	si.StartupInfo.cb = sizeof(si);
	si.StartupInfo.hStdOutput = hStdOutWrite;
	si.StartupInfo.hStdError = hStdErrWrite;
	si.StartupInfo.hStdInput = hStdIn;
	si.StartupInfo.dwFlags = STARTF_USESTDHANDLES;

	// Node/VS Code内には拡張機能と無関係な継承可能ハンドルが存在し得る。
	// 子へ渡す対象をstdin/stdout/stderrだけに限定し、情報露出と、子が別の
	// pipe/fileを保持して親の終了を妨げる事象を防ぐ。
	SIZE_T attributeBytes = 0;
	InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
	std::vector<unsigned char> attributeStorage(attributeBytes);
	si.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
		attributeStorage.data());
	if (attributeBytes == 0
		|| !InitializeProcThreadAttributeList(
			si.lpAttributeList, 1, 0, &attributeBytes)) {
		const DWORD setupError = GetLastError();
		CloseHandle(hStdIn);
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		CloseHandle(hStdErrRead);
		CloseHandle(hStdErrWrite);
		if (captureOutput) {
			result.error = "Cannot initialize child handle list: "
				+ std::to_string(setupError);
		}
		return result;
	}
	HANDLE inheritedHandles[] = { hStdIn, hStdOutWrite, hStdErrWrite };
	if (!UpdateProcThreadAttribute(
		si.lpAttributeList, 0,
		PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
		inheritedHandles, sizeof(inheritedHandles), nullptr, nullptr)) {
		const DWORD setupError = GetLastError();
		DeleteProcThreadAttributeList(si.lpAttributeList);
		CloseHandle(hStdIn);
		CloseHandle(hStdOutRead);
		CloseHandle(hStdOutWrite);
		CloseHandle(hStdErrRead);
		CloseHandle(hStdErrWrite);
		if (captureOutput) {
			result.error = "Cannot restrict child handle inheritance: "
				+ std::to_string(setupError);
		}
		return result;
	}

	PROCESS_INFORMATION pi{};
	std::wstring cmdLine = L"\"" + Utils::utf8ToWide(command) + L"\" "
		+ Utils::utf8ToWide(args);
	std::wstring wideWorkingDir = workingDir.empty()
		? std::wstring() : Utils::utf8ToWide(workingDir);

	HANDLE job = CreateJobObjectW(nullptr, nullptr);
	bool jobConfigured = false;
	if (job) {
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
		limits.BasicLimitInformation.LimitFlags =
			JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		jobConfigured = SetInformationJobObject(job,
			JobObjectExtendedLimitInformation, &limits, sizeof(limits)) != FALSE;
	}

	BOOL ok = CreateProcessW(
		nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
		CREATE_NO_WINDOW | CREATE_SUSPENDED
			| EXTENDED_STARTUPINFO_PRESENT,
		nullptr,
		wideWorkingDir.empty() ? nullptr : wideWorkingDir.c_str(),
		&si.StartupInfo, &pi);
	const DWORD createError = ok ? ERROR_SUCCESS : GetLastError();
	DeleteProcThreadAttributeList(si.lpAttributeList);

	// 親側の書込端は閉じる。子側だけが書込端を持つ状態にしておかないと、
	// 子が exit しても ReadFile が EOF を返さずスレッドが終わらない。
	CloseHandle(hStdIn);
	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);

	if (!ok) {
		if (job) CloseHandle(job);
		CloseHandle(hStdOutRead);
		CloseHandle(hStdErrRead);
		if (captureOutput) {
			result.error = "CreateProcess failed: "
				+ std::to_string(createError);
		}
		return result;
	}

	const bool jobAssigned = jobConfigured
		&& AssignProcessToJobObject(job, pi.hProcess) != FALSE;
	if (ResumeThread(pi.hThread) == static_cast<DWORD>(-1)) {
		if (jobAssigned) TerminateJobObject(job, ERROR_PROCESS_ABORTED);
		else TerminateProcess(pi.hProcess, ERROR_PROCESS_ABORTED);
	}

	bool stdoutTruncated = false;
	bool stderrTruncated = false;
	std::thread outThread([&] {
		drainPipeBlocking(hStdOutRead, result.output,
			captureOutput, stdoutTruncated);
	});
	std::thread errThread([&] {
		drainPipeBlocking(hStdErrRead, result.error,
			captureOutput, stderrTruncated);
	});

	const DWORD waitResult = WaitForSingleObject(pi.hProcess, timeoutMs);
	if (waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED) {
		result.timedOut = waitResult == WAIT_TIMEOUT;
		const DWORD terminationCode = waitResult == WAIT_TIMEOUT
			? ERROR_TIMEOUT : ERROR_PROCESS_ABORTED;
		if (jobAssigned) TerminateJobObject(job, terminationCode);
		else TerminateProcess(pi.hProcess, terminationCode);
		WaitForSingleObject(pi.hProcess, 5000);
		// Jobへ参加できない環境で孫プロセスがパイプを保持していても、
		// readerスレッドを永久に待たない。
		CancelSynchronousIo(outThread.native_handle());
		CancelSynchronousIo(errThread.native_handle());
	}
	if (job) CloseHandle(job);
	outThread.join();
	errThread.join();
	result.outputTruncated = stdoutTruncated || stderrTruncated;

	DWORD exitCode = 0;
	if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
		result.exitCode = static_cast<int>(exitCode);
	}

	CloseHandle(hStdOutRead);
	CloseHandle(hStdErrRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	if (captureOutput) {
		result.output = Utils::normalizeToUtf8(result.output);
		result.error = Utils::normalizeToUtf8(result.error);
		if (result.outputTruncated) {
			result.error += "\n[Monocon: process output was truncated at 16 MiB]\n";
		}
		if (result.timedOut) {
			result.error += "\n[Monocon: child process timed out]\n";
		}
	}

	return result;
}
