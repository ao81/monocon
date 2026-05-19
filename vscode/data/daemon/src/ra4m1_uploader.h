#pragma once

#include <string>
#include <vector>
#include <cstdint>

// =============================================================================
// RA4M1 ネイティブアップローダ (Arduino UNO R4 WiFi 専用、DFU 1.1)
//
// Renesas RA4M1 (Cortex-M4) は工場出荷時に Arduino LLC が用意した
// USB DFU ブートローダが ROM 領域 (0x0000-0x3FFF) に焼かれている。
//
// 書き込みフロー:
//   1. アプリ側のシリアル CDC を 1200bps で open→close する
//      → MCU 内のアプリが DFU モードへの遷移コードを実行 → USB 再列挙
//   2. PC 側で旧 COM ポートが消えるのを待つ (最短検出)
//   3. PC 側で VID:PID=2341:006D の DFU デバイスが出現するのを待つ
//   4. dfu-util を最小引数で叩いて 0x4000 以降にユーザーフラッシュを書き込み
//   5. dfu-util の -R / :leave で DFU からアプリへ即復帰
//
// 設計方針 (高速優先):
//   - シリアル open / DTR トグル / Sleep を最小化 (旧 STK500v2 と同じ哲学)
//   - dfu-util への引数は固定構成。--reset-stm32 等の余計なオプションは省く
//   - .hex を介さず .bin を直接渡す (objcopy の二重実行を避ける)
// =============================================================================
namespace Ra4m1Uploader {

	struct UploadStats {
		bool success = false;
		std::string errorMessage;
		// 内訳 (デバッグ用)
		double touchMs = 0;        // 1200bps タッチ
		double waitPortGoneMs = 0; // アプリ COM 消失待ち
		double waitDfuMs = 0;      // DFU デバイス出現待ち
		double dfuMs = 0;          // dfu-util プロセス実行
		double totalMs = 0;
		size_t bytesWritten = 0;
		std::string appPort;       // アプリモード COM
		std::string bootloaderId;  // 検出した DFU デバイスの "VID:PID"
		std::string dfuOutput;     // dfu-util の標準出力 (エラー時のみ重要)
	};

	// .elf から .bin を生成するためのヘルパ (arm-none-eabi-objcopy 経由)。
	// 成功すれば true、失敗すれば err に詳細を入れて false。
	bool generateBin(const std::string& objcopyExe,
		const std::string& elfPath,
		const std::string& binPath,
		std::string& err);

	// UNO R4 WiFi へアップロード。
	//   appPort:       現在アプリモードで認識されている COM ポート ("COM3")
	//   binPath:       書き込む .bin ファイル (絶対パス)
	//   dfuUtilExe:    dfu-util.exe の絶対パス
	UploadStats uploadUnoR4Wifi(const std::string& appPort,
		const std::string& binPath,
		const std::string& dfuUtilExe);

} // namespace Ra4m1Uploader
