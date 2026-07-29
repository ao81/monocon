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

	// Arduino互換VIDまたは説明で一意に絞れたポートを返す。
	// 見つからない場合と複数候補で曖昧な場合は空文字。
	std::string findArduinoPort();

	// Arduino MEGA / ADK固有PID、または一般的なUSBシリアル変換ICを判定。
	bool isArduinoVidPid(unsigned short vid, unsigned short pid);

} // namespace PortScanner
