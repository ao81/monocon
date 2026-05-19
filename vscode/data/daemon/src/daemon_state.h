#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <atomic>

// =============================================================================
// DaemonState: デーモン全体で共有するホットキャッシュ
//   - ツールチェーンパス (起動時 1 回だけ解決して保持)
//     - arm-none-eabi-gcc/g++/objcopy (Renesas RA4M1 = Cortex-M4 用)
//     - dfu-util (Uno R4 WiFi の DFU アップロード用)
//   - COM ポートリスト (レジストリ通知で更新)
//   - スケッチ単位のインクリメンタル状態
//   - core.a グローバルキャッシュ
// =============================================================================
struct ToolchainPaths {
	// === ARM ツールチェーン (Renesas RA4M1 / Cortex-M4) ===
	std::string armGpp;        // arm-none-eabi-g++.exe
	std::string armGcc;        // arm-none-eabi-gcc.exe
	std::string armObjcopy;    // arm-none-eabi-objcopy.exe
	std::string armAr;         // arm-none-eabi-ar.exe
	std::string armSize;       // arm-none-eabi-size.exe (情報表示用、任意)

	// === DFU アップローダ ===
	std::string dfuUtil;       // dfu-util.exe

	// === Arduino renesas_uno コア / variants / FSP ===
	std::string coreDir;       // .../hardware/renesas_uno/<ver>/cores/arduino
	std::string variantDir;    // .../variants/UNOWIFIR4
	std::string includeDirs;   // FSP / RA / ESP ホスト用などの追加 include を ";" 区切りで保持
	std::string linkerScript;  // variants/UNOWIFIR4/linker_scripts/gcc/fsp.ld

	std::string compilerVersion;     // arm-none-eabi-gcc --version の最初の行
	std::string boardPackageVersion; // arduino:renesas_uno のバージョン文字列

	bool valid = false;
	std::string errorMessage;
};

// スケッチ単位の最終ビルド結果メタデータ (差分判定はディスク上の .stamps で行う)
struct SketchBuildState {
	std::string sketchDir;
	std::string buildDir;
	std::string hexFile;          // arm-none-eabi-objcopy -O ihex 出力 (互換用)
	std::string binFile;          // arm-none-eabi-objcopy -O binary 出力 (dfu-util に渡す本命)
	std::string elfFile;
	std::chrono::steady_clock::time_point lastBuildAt{};
	bool hasValidBuild = false;
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
	std::string version = "2.0.0-unor4wifi";
};

// グローバルインスタンス
extern DaemonState g_state;

// 初期化 (起動時に 1 回だけ呼ぶ)
//   失敗したら toolchain.valid = false で詳細は errorMessage に入る
bool initializeDaemonState();

// COM ポートを再スキャン (レジストリ → メモリへ)
void refreshComPorts();
