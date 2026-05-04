#include "pipe_io.h"

#include <sddl.h>
#include <vector>

#pragma comment(lib, "advapi32.lib")

namespace {

	// 現在ユーザーの SID 文字列を取得
	std::string currentUserSidString() {
		HANDLE hToken = nullptr;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) return "unknown";

		DWORD len = 0;
		GetTokenInformation(hToken, TokenUser, nullptr, 0, &len);
		std::vector<BYTE> buf(len);
		std::string out = "unknown";
		if (GetTokenInformation(hToken, TokenUser, buf.data(), len, &len)) {
			auto* tu = reinterpret_cast<TOKEN_USER*>(buf.data());
			LPSTR sidStr = nullptr;
			if (ConvertSidToStringSidA(tu->User.Sid, &sidStr)) {
				out = sidStr;
				LocalFree(sidStr);
			}
		}
		CloseHandle(hToken);
		return out;
	}

} // namespace

namespace PipeIO {

	std::string makePipeName() {
		return std::string("\\\\.\\pipe\\arduino-build-") + currentUserSidString();
	}

	std::string makeMutexName() {
		// Local\ = ターミナルセッションローカル。同じ Windows ログオン内で唯一。
		return std::string("Local\\arduino-build-daemon-") + currentUserSidString();
	}

	// ---------------------------------------------------------------------
	// 読み出し
	// ---------------------------------------------------------------------
	std::string readMessage(HANDLE hPipe) {
		// 1) ヘッダ部 (\r\n\r\n まで) を 1 バイトずつ読む
		std::string header;
		char ch = 0;
		while (true) {
			DWORD n = 0;
			BOOL ok = ReadFile(hPipe, &ch, 1, &n, nullptr);
			if (!ok || n == 0) return "";
			header.push_back(ch);
			if (header.size() >= 4 &&
				header.compare(header.size() - 4, 4, "\r\n\r\n") == 0) break;
			if (header.size() > 4096) return ""; // ヘッダが暴走している
		}

		// 2) Content-Length をパース
		size_t pos = header.find("Content-Length:");
		if (pos == std::string::npos) return "";
		size_t lineEnd = header.find("\r\n", pos);
		if (lineEnd == std::string::npos) return "";
		std::string lenStr = header.substr(pos + 15, lineEnd - (pos + 15));
		// trim
		while (!lenStr.empty() && (lenStr.front() == ' ' || lenStr.front() == '\t')) lenStr.erase(0, 1);
		while (!lenStr.empty() && (lenStr.back() == ' ' || lenStr.back() == '\t')) lenStr.pop_back();
		size_t len = 0;
		try { len = std::stoul(lenStr); } catch (...) { return ""; }
		if (len == 0 || len > 64 * 1024 * 1024) return ""; // 上限 64MB

		// 3) ボディを len バイト読む
		std::string body(len, '\0');
		size_t total = 0;
		while (total < len) {
			DWORD n = 0;
			BOOL ok = ReadFile(hPipe, body.data() + total,
				static_cast<DWORD>(len - total), &n, nullptr);
			if (!ok || n == 0) return "";
			total += n;
		}
		return body;
	}

	// ---------------------------------------------------------------------
	// 書き込み
	// ---------------------------------------------------------------------
	bool writeMessage(HANDLE hPipe, const std::string& body) {
		std::string header = "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";

		auto writeAll = [&](const char* p, size_t n) {
			size_t total = 0;
			while (total < n) {
				DWORD written = 0;
				BOOL ok = WriteFile(hPipe, p + total,
					static_cast<DWORD>(n - total), &written, nullptr);
				if (!ok || written == 0) return false;
				total += written;
			}
			return true;
			};

		if (!writeAll(header.data(), header.size())) return false;
		if (!writeAll(body.data(), body.size())) return false;
		FlushFileBuffers(hPipe);
		return true;
	}

} // namespace PipeIO