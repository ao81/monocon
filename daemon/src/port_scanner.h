#pragma once

#include <string>
#include <vector>

// =============================================================================
// PortScanner: COM ポート列挙 (WMI を使わない高速実装)
//
// HKLM\HARDWARE\DEVICEMAP\SERIALCOMM レジストリを直接読むだけ。
// 典型実行時間 1〜10ms。WMI の数百ms〜数秒コールドスタートを完全回避。
//
// Arduino MEGA / ADK の VID:PID マッチング機能も提供する。
// =============================================================================
namespace PortScanner {

	struct PortInfo {
		std::string name;        // "COM3"
		std::string description; // レジストリのフレンドリ名 (取得できれば)
	};

	// 全シリアルポートを列挙
	std::vector<PortInfo> listAll();

	// 名前だけのリスト ("COM3", "COM5", ...)
	std::vector<std::string> listNames();

	// Arduino っぽい VID:PID で絞り込んだ最初のポートを返す
	// 見つからなければ空文字
	std::string findArduinoPort();

	// Arduino MEGA / ADK の典型 VID:PID
	// 0x2341 (Arduino LLC)
	// 0x2A03 (Arduino SRL)
	bool isArduinoVidPid(unsigned short vid, unsigned short pid);

} // namespace PortScanner