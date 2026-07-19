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
//   - インクリメンタルパスは avr-g++ / avr-gcc / avr-objcopy を直接呼ぶ。
//   - 全状態は g_state に持つ (初回のみツールチェーン解決)
// =============================================================================
namespace Builder {

	struct CompileRequest {
		std::string sketchDir;     // 必須
		std::string workspaceDir;  // 任意。空なら .vscode/ を辿って自動検出
		std::string fqbn;          // 例: "arduino:avr:mega" (未指定なら mega)
		bool forceFullBuild = false;
	};

	struct CompileResult {
		bool success = false;
		bool cached = false;       // ソース変更なしでスキップしたか
		int  recompiledFiles = 0;  // 今回コンパイルしたファイル数
		int  totalFiles = 0;       // スケッチ内の総ソースファイル数
		std::string hexFile;
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
		bool forceFullUpload = false; // trueなら直前イメージとの差分判定を使わない
	};

	struct UploadResult {
		bool success = false;
		bool cached = false;       // 同一ボード・同一HEXのため実転送を省略したか
		std::string port;
		double uploadTimeMs = 0;
		std::string errorMessage;
		std::string avrdudeOutput;
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
