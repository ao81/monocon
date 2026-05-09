#pragma once

// windows.h を間接的に引き込むヘッダより前に NOMINMAX を定義しておく
// これにより min/max マクロと std::max/std::min の衝突を全翻訳単位で防ぐ

#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <stdexcept>

namespace fs = std::filesystem;

// =============================================================================
// Utils: ファイルシステム / 文字列 / プロセスのユーティリティ
// =============================================================================
namespace Utils {

	// --- パス取得 ---
	std::string getLocalAppDataPath();
	std::string getGlobalCacheDir();      // %LOCALAPPDATA%\ArduinoBuildDaemon

	// --- ファイル / ディレクトリ ---
	bool fileExists(const std::string& path);
	bool directoryExists(const std::string& path);

	// --- ファイル I/O ---
	std::string readFile(const std::string& path);
	std::vector<std::string> readLines(const std::string& path);
	void writeFile(const std::string& path, const std::string& content);
	void writeLines(const std::string& path, const std::vector<std::string>& lines);
	bool writeFileIfChanged(const std::string& path, const std::string& content);

	// --- ディレクトリ操作 ---
	void createDirectory(const std::string& path);
	void deleteFile(const std::string& path);
	void cleanDirectory(const std::string& dir, const std::string& extension);

	// --- パス操作 ---
	std::string joinPath(const std::string& base, const std::string& relative);
	std::string getFileName(const std::string& path);
	std::string getFileStem(const std::string& path);
	std::string getParentDirectory(const std::string& path);

	// --- ファイル列挙 ---
	std::vector<std::string> getFilesByExtension(const std::string& dir, const std::string& extension);
	std::vector<std::string> getDirectories(const std::string& dir);
	std::string getLatestDirectory(const std::string& baseDir);

	// --- ハッシュ / タイムスタンプ ---
	long long getFileLastWriteTime(const std::string& path);
	std::string getSourceSignature(const std::vector<std::string>& files);

	// --- 文字列操作 ---
	std::string trim(const std::string& str);
	std::vector<std::string> split(const std::string& str, char delimiter);
	std::string replaceAll(std::string str, const std::string& from, const std::string& to);

	// --- ハッシュ ---
	// 短い文字列なら sha1 を hex 16 文字くらいに切り詰めるだけで十分
	std::string sha1Hex(const std::string& data);

} // namespace Utils

// =============================================================================
// Stopwatch
// =============================================================================
class Stopwatch {
public:
	Stopwatch();
	void start();
	void stop();
	void restart();
	double elapsedMilliseconds() const;
	double elapsedSeconds() const;
private:
	std::chrono::steady_clock::time_point start_time_;
	std::chrono::steady_clock::time_point end_time_;
	bool running_ = false;
};

// =============================================================================
// Process
// =============================================================================
struct ProcessResult {
	int exitCode = -1;
	std::string output;   // stdout
	std::string error;    // stderr
};

ProcessResult runProcess(const std::string& command,
	const std::string& args,
	const std::string& workingDir = "",
	bool captureOutput = true);