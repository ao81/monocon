#pragma once

#include <string>
#include <nlohmann/json.hpp>

// =============================================================================
// Builder: スケッチのコンパイル/アップロード本体
//
// 設計方針:
//   - フルビルドパスは arduino-cli に委譲 (初回 core.a 生成)
//     ただし core.a は %LOCALAPPDATA%\ArduinoBuildDaemon\core-cache に永続化
//     し、以後の同 fqbn+flags ビルドで再利用する。
//   - インクリメンタルパスは arm-none-eabi-g++ / arm-none-eabi-gcc /
//     arm-none-eabi-objcopy を直接呼ぶ。
//   - 全状態は g_state に持つ (初回のみツールチェーン解決)
//
// 対応ボード:
//   - Arduino UNO R4 WiFi (Renesas RA4M1 / Cortex-M4) のみ
//     fqbn デフォルト: "arduino:renesas_uno:unor4wifi"
// =============================================================================
namespace Builder {

	struct CompileRequest {
		std::string sketchDir;     // 必須
		std::string workspaceDir;  // 任意。空なら .vscode/ を辿って自動検出
		std::string fqbn;          // 例: "arduino:renesas_uno:unor4wifi" (未指定なら unor4wifi)
		bool forceFullBuild = false;
	};

	struct CompileResult {
		bool success = false;
		bool cached = false;       // ソース変更なしでスキップしたか
		int  recompiledFiles = 0;  // 今回コンパイルしたファイル数
		int  totalFiles = 0;       // スケッチ内の総ソースファイル数
		std::string hexFile;       // 互換用 (Intel HEX)
		std::string binFile;       // dfu-util に渡す生バイナリ
		std::string elfFile;
		double buildTimeMs = 0;
		std::string errorMessage;
		std::string compilerOutput;
	};

	struct UploadRequest {
		std::string sketchDir;
		std::string workspaceDir;  // 任意。空なら .vscode/ を辿って自動検出
		std::string port;          // 空なら自動検出
		bool skipCompile = false;
	};

	struct UploadResult {
		bool success = false;
		std::string port;
		double uploadTimeMs = 0;
		std::string errorMessage;
		std::string dfuOutput;     // dfu-util の出力 (旧 avrdudeOutput 相当)
		// アップロード前に走らせたコンパイルの結果
		CompileResult compile;
	};

	// === エントリポイント ===
	CompileResult compile(const CompileRequest& req);
	UploadResult  upload(const UploadRequest& req);

	// === JSON-RPC 用シリアライズ ===
	nlohmann::json toJson(const CompileResult& r);
	nlohmann::json toJson(const UploadResult& r);

} // namespace Builder
