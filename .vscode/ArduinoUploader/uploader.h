#pragma once

#include <memory>
#include <string>

// =============================================================================
// Uploader: Arduino スケッチのコンパイル/書き込み
//
// - 初回ビルド時は arduino-cli を呼んで core.a を含む完全ビルドを行い、
//   結果を .vscode/build/<sketch> 配下にキャッシュする。
// - 2回目以降はソースのタイムスタンプを比較し、変更があれば
//   avr-g++ / avr-gcc / avr-objcopy を直接呼ぶインクリメンタルビルド。
// - 変更が無ければビルドはスキップして avrdude で書き込みのみ。
// =============================================================================
class Uploader {
public:
	Uploader(const std::string& sketchDir, const std::string& port);
	~Uploader();

	Uploader(const Uploader&) = delete;
	Uploader& operator=(const Uploader&) = delete;

	bool initialize();
	bool upload();
	std::string getLastError() const;

private:
	struct Impl;
	std::unique_ptr<Impl> pImpl;

	// --- 初期化フェーズ ---
	bool resolveToolchainPaths();
	bool resolvePort();
	bool resolveBuildPaths();
	bool checkCache();

	// --- ビルドフェーズ ---
	bool compile();
	bool fullCompile();
	bool incrementalCompile();
	bool uploadToBoard();

	// --- ヘルパ ---
	bool resolveAvrdudePaths();
	std::string resolveCachedDirectory(const std::string& cacheFile,
		const std::string& baseDir,
		const std::string& errorMessage,
		const std::string& suffix = "");
	std::string findArduinoPort();
};