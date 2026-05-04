#include "port_scanner.h"

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>

#include <algorithm>
#include <cctype>
#include <cstdio>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "advapi32.lib")

namespace {

	// ---------------------------------------------------------------------
	// HKLM\HARDWARE\DEVICEMAP\SERIALCOMM の値を全部読む
	//   value name : "\Device\Serial0" 等のドライバ内部名 (使わない)
	//   value data : "COM3" 等のポート名
	// SerialPort.GetPortNames() と等価。HDD コールドでも 1〜10ms。
	// ---------------------------------------------------------------------
	std::vector<std::string> readSerialCommRegistry() {
		std::vector<std::string> names;
		HKEY hKey = nullptr;
		LONG r = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
			"HARDWARE\\DEVICEMAP\\SERIALCOMM",
			0, KEY_READ, &hKey);
		if (r != ERROR_SUCCESS) return names;

		DWORD index = 0;
		char valueName[256];
		BYTE valueData[256];
		while (true) {
			DWORD nameLen = sizeof(valueName);
			DWORD dataLen = sizeof(valueData);
			DWORD type = 0;
			LONG e = RegEnumValueA(hKey, index++, valueName, &nameLen,
				nullptr, &type, valueData, &dataLen);
			if (e == ERROR_NO_MORE_ITEMS) break;
			if (e != ERROR_SUCCESS) break;
			if (type != REG_SZ) continue;

			// dataLen はバイト数。NUL 終端を含む可能性があるので除去。
			size_t len = dataLen;
			while (len > 0 && (valueData[len - 1] == 0)) --len;
			std::string portName(reinterpret_cast<char*>(valueData), len);
			if (!portName.empty()) names.push_back(portName);
		}
		RegCloseKey(hKey);

		std::sort(names.begin(), names.end());
		names.erase(std::unique(names.begin(), names.end()), names.end());
		return names;
	}

	// ---------------------------------------------------------------------
	// SetupAPI で各 COM ポートデバイスの HardwareID (VID/PID) を取得
	// ---------------------------------------------------------------------
	struct DeviceMatch {
		std::string portName;     // "COM3"
		std::string description;  // "Arduino Mega 2560 (COM3)" 等
		unsigned short vid = 0;
		unsigned short pid = 0;
	};

	// HardwareID 文字列から "USB\VID_xxxx&PID_yyyy" を解析
	bool parseVidPid(const std::string& hwid, unsigned short& vid, unsigned short& pid) {
		size_t v = hwid.find("VID_");
		size_t p = hwid.find("PID_");
		if (v == std::string::npos || p == std::string::npos) return false;
		unsigned int vv = 0, pp = 0;
		if (sscanf_s(hwid.c_str() + v + 4, "%4x", &vv) != 1) return false;
		if (sscanf_s(hwid.c_str() + p + 4, "%4x", &pp) != 1) return false;
		vid = static_cast<unsigned short>(vv);
		pid = static_cast<unsigned short>(pp);
		return true;
	}

	std::vector<DeviceMatch> enumerateSerialDevices() {
		std::vector<DeviceMatch> result;

		HDEVINFO hDevInfo = SetupDiGetClassDevsA(
			&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
		if (hDevInfo == INVALID_HANDLE_VALUE) return result;

		SP_DEVINFO_DATA devData{};
		devData.cbSize = sizeof(devData);

		for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devData); ++i) {
			DeviceMatch m;

			// "PortName" レジストリ値で COMxx を取得
			HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devData,
				DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
			if (hKey != INVALID_HANDLE_VALUE) {
				char buf[64] = { 0 };
				DWORD len = sizeof(buf);
				DWORD type = 0;
				if (RegQueryValueExA(hKey, "PortName", nullptr, &type,
					reinterpret_cast<BYTE*>(buf), &len) == ERROR_SUCCESS) {
					m.portName = buf;
				}
				RegCloseKey(hKey);
			}
			if (m.portName.empty()) continue;

			// FriendlyName / DeviceDesc
			char desc[512] = { 0 };
			DWORD descSize = sizeof(desc);
			if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData,
				SPDRP_FRIENDLYNAME, nullptr, reinterpret_cast<BYTE*>(desc),
				descSize, nullptr)) {
				m.description = desc;
			} else if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData,
				SPDRP_DEVICEDESC, nullptr, reinterpret_cast<BYTE*>(desc),
				descSize, nullptr)) {
				m.description = desc;
			}

			// HardwareID から VID/PID
			char hwid[1024] = { 0 };
			if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData,
				SPDRP_HARDWAREID, nullptr, reinterpret_cast<BYTE*>(hwid),
				sizeof(hwid), nullptr)) {
				parseVidPid(hwid, m.vid, m.pid);
			}

			result.push_back(std::move(m));
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);
		return result;
	}

} // namespace

namespace PortScanner {

	bool isArduinoVidPid(unsigned short vid, unsigned short pid) {
		// Arduino LLC / SRL の VID
		// MEGA 2560 R3:        2341:0042
		// MEGA ADK R3:         2341:0044
		// MEGA 2560 (CDC):     2341:0010
		// クローン互換:        1A86:7523 (CH340), 0403:6001 (FTDI), 10C4:EA60 (CP210x)
		(void)pid;
		switch (vid) {
		case 0x2341: // Arduino LLC
		case 0x2A03: // Arduino SRL
		case 0x1A86: // QinHeng (CH340 クローン)
		case 0x0403: // FTDI
		case 0x10C4: // Silicon Labs (CP210x)
			return true;
		default:
			return false;
		}
	}

	std::vector<PortInfo> listAll() {
		// レジストリで COM 名を取得し、SetupAPI で description を補完
		auto names = readSerialCommRegistry();
		auto devs = enumerateSerialDevices();

		std::vector<PortInfo> out;
		out.reserve(names.size());
		for (const auto& n : names) {
			PortInfo p;
			p.name = n;
			for (const auto& d : devs) {
				if (d.portName == n) {
					p.description = d.description;
					break;
				}
			}
			out.push_back(std::move(p));
		}
		return out;
	}

	std::vector<std::string> listNames() {
		return readSerialCommRegistry();
	}

	std::string findArduinoPort() {
		auto devs = enumerateSerialDevices();
		// VID マッチを優先
		for (const auto& d : devs) {
			if (isArduinoVidPid(d.vid, d.pid)) return d.portName;
		}
		// フォールバック: description に "Arduino" を含むもの
		for (const auto& d : devs) {
			std::string lower = d.description;
			std::transform(lower.begin(), lower.end(), lower.begin(),
				[](unsigned char c) { return std::tolower(c); });
			if (lower.find("arduino") != std::string::npos) return d.portName;
		}
		// それでも無ければ「最初に見つかった COM ポート」
		auto names = readSerialCommRegistry();
		if (!names.empty()) return names.front();
		return "";
	}

} // namespace PortScanner