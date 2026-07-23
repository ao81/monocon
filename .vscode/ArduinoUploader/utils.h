#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <stdexcept>

namespace fs = std::filesystem;

// =============================================================================
// Utils: ファイルシステム/文字列操作のユーティリティ
// =============================================================================
namespace Utils {

	// --- パス取得 ---
	std::string getLocalAppDataPath();
	std::string getGlobalCacheDir();

	// --- ファイル/ディレクトリの存在チェック ---
	bool fileExists(const std::string& path);
	bool directoryExists(const std::string& path);

	// --- ファイル I/O ---
	std::string readFile(const std::string& path);
	std::vector<std::string> readLines(const std::string& path);
	void writeFile(const std::string& path, const std::string& content);
	void writeLines(const std::string& path, const std::vector<std::string>& lines);

	// 内容が変わっている時だけ書き込む(I/Oを節約)
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
	// 指定された拡張子(例: ".h")のファイルを取得。拡張子は完全一致。
	std::vector<std::string> getFilesByExtension(const std::string& dir, const std::string& extension);
	std::vector<std::string> getDirectories(const std::string& dir);
	std::string getLatestDirectory(const std::string& baseDir);

	// --- ハッシュ/タイムスタンプ ---
	// ファイル群の最終更新時刻からシグネチャ文字列を生成。
	std::string getSourceSignature(const std::vector<std::string>& files);
	long long getFileLastWriteTime(const std::string& path);

	// --- 文字列操作 ---
	std::string trim(const std::string& str);
	std::vector<std::string> split(const std::string& str, char delimiter);
	std::string replaceAll(std::string str, const std::string& from, const std::string& to);

} // namespace Utils

// =============================================================================
// Stopwatch: 経過時間計測
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
// Process: 外部プロセス実行
// =============================================================================
struct ProcessResult {
	int exitCode = -1;
	std::string output;  // stdout
	std::string error;   // stderr
};

// 外部プロセスを実行して終了を待つ。
// stdout/stderr を非ブロッキングに読み続けるためデッドロックしない。
// 出力をすべて捨てたい場合は captureOutput を false にすると軽くなる。
ProcessResult runProcess(const std::string& command,
	const std::string& args,
	const std::string& workingDir = "",
	bool captureOutput = true);