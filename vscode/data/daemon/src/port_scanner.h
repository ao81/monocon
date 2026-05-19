#pragma once

#include <string>
#include <vector>

// =============================================================================
// PortScanner: COM ポート列挙 (WMI を使わない高速実装)
//
// HKLM\HARDWARE\DEVICEMAP\SERIALCOMM レジストリを直接読むだけ。
// 典型実行時間 1〜10ms。WMI の数百ms〜数秒コールドスタートを完全回避。
//
// Arduino Uno R4 WiFi (Renesas RA4M1) の VID:PID マッチング機能も提供する。
// =============================================================================
namespace PortScanner {

	struct PortInfo {
		std::string name;        // "COM3"
		std::string description; // レジストリのフレンドリ名 (取得できれば)
		unsigned short vid = 0;  // USB VID (取得できれば)
		unsigned short pid = 0;  // USB PID (取得できれば)
	};

	// 全シリアルポートを列挙
	std::vector<PortInfo> listAll();

	// 名前だけのリスト ("COM3", "COM5", ...)
	std::vector<std::string> listNames();

	// VID/PID 付きで全列挙 (1200bps タッチ後の DFU 出現待ちなど用)
	std::vector<PortInfo> listAllWithIds();

	// Uno R4 WiFi っぽい VID:PID で絞り込んだ最初のポートを返す
	// 見つからなければ空文字
	std::string findArduinoPort();

	// Uno R4 WiFi の DFU/Bootloader モード VID:PID で絞り込んだ最初のポートを返す
	// (1200bps タッチ後にここから取得して dfu-util に渡す)
	std::string findBootloaderPort();

	// Arduino Uno R4 WiFi の典型 VID:PID
	// アプリモード:   0x2341:0x1002
	// Bootloader:    0x2341:0x006D (DFU)
	// 互換クローン: CH340 / FTDI / CP210x (Uno R4 互換は通常 1002 を名乗る)
	bool isArduinoVidPid(unsigned short vid, unsigned short pid);

	// Uno R4 WiFi の DFU/Bootloader モードか (VID=0x2341 / PID=0x006D)
	bool isBootloaderVidPid(unsigned short vid, unsigned short pid);

} // namespace PortScanner
