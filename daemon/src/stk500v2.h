#pragma once

#include <string>
#include <vector>
#include <cstdint>

// =============================================================================
// STK500v2 ネイティブアップローダ (Mega 2560 専用、115200 baud)
//
// avrdude を排除して、stk500v2 プロトコルを直接話す。
//   - プロセス起動オーバーヘッドゼロ
//   - avrdude.conf パース不要
//   - 期待値: 500-700ms (avrdude 経由は 1400ms+)
// =============================================================================
namespace Stk500v2 {

	struct UploadStats {
		bool success = false;
		std::string errorMessage;
		// 内訳 (デバッグ用)
		double openMs = 0;
		double resetMs = 0;
		double syncMs = 0;
		double progMs = 0;
		double leaveMs = 0;
		double totalMs = 0;
		size_t bytesWritten = 0;
		size_t pagesWritten = 0;
	};

	// Intel HEX ファイルを読み込み、フラッシュイメージを返す。
	// 未使用領域は 0xFF。Mega 2560 のフラッシュサイズ (256KB) 分確保。
	bool readIntelHex(const std::string& path,
		std::vector<uint8_t>& flash,
		std::string& errOut);

	// Mega 2560 へアップロード
	UploadStats uploadMega2560(const std::string& port,
		const std::vector<uint8_t>& flash);

} // namespace Stk500v2