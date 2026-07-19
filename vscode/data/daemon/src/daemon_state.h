#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <memory>

// =============================================================================
// DaemonState: デーモン全体で共有するホットキャッシュ
//   - ツールチェーンパス (起動時 1 回だけ解決して保持)
//   - COM ポートリスト (レジストリ通知で更新)
//   - スケッチ単位のインクリメンタル状態
//   - core.a グローバルキャッシュ
// =============================================================================
struct ToolchainPaths {
	std::string avrGpp;        // avr-g++.exe
	std::string avrGcc;        // avr-gcc.exe
	std::string avrObjcopy;    // avr-objcopy.exe
	std::string avrAr;         // avr-ar.exe
	std::string avrdude;       // avrdude.exe
	std::string avrdudeConf;   // avrdude.conf

	std::string coreDir;       // ArduinoCore-avr の cores/arduino
	std::string variantDir;    // variants/mega
	std::string compilerVersion; // avr-gcc --version の最初の行

	bool valid = false;
	std::string errorMessage;
};

// スケッチ単位の最終ビルド結果メタデータ (差分判定はディスク上の .stamps で行う)
struct SketchBuildState {
	std::string sketchDir;
	std::string buildDir;
	std::string hexFile;
	std::string elfFile;
	std::chrono::steady_clock::time_point lastBuildAt{};
	bool hasValidBuild = false;
	// 同じHEXを繰り返し書き込む際、Intel HEX解析とディスクI/Oを省略する。
	std::shared_ptr<const std::vector<uint8_t>> flashImage;
	long long flashHexMtime = 0;
	uint64_t flashHexSize = 0;
};

struct DaemonState {
	// --- ツールチェーン (起動時 1 回のみ初期化) ---
	ToolchainPaths toolchain;

	// --- COM ポートキャッシュ ---
	std::mutex portMtx;
	std::vector<std::string> cachedPorts;
	std::string cachedArduinoPort;
	std::chrono::steady_clock::time_point portsCachedAt{};

	// --- スケッチ単位の状態 (キー: sketchDir の正規化パス) ---
	std::mutex sketchMtx;
	std::unordered_map<std::string, SketchBuildState> sketches;

	// --- core.a グローバルキャッシュのルート ---
	std::string coreCacheRoot; // %LOCALAPPDATA%\ArduinoBuildDaemon\core-cache

	// --- ライフサイクル ---
	std::atomic<bool> shutdownRequested{ false };
	std::chrono::steady_clock::time_point startedAt{};
	std::chrono::steady_clock::time_point lastRequestAt{};
	std::atomic<long long> requestCount{ 0 };

	// --- バージョン ---
	std::string version = "1.3.0";
};

// グローバルインスタンス
extern DaemonState g_state;

// 初期化 (起動時に 1 回だけ呼ぶ)
//   失敗したら toolchain.valid = false で詳細は errorMessage に入る
bool initializeDaemonState();

// COM ポートを再スキャン (レジストリ → メモリへ)
void refreshComPorts();
