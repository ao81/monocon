#include "port_scanner.h"
#include "utils.h"

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

	std::wstring readDeviceProperty(HDEVINFO devices,
		SP_DEVINFO_DATA& device, DWORD property) {
		DWORD type = 0;
		DWORD requiredBytes = 0;
		SetupDiGetDeviceRegistryPropertyW(
			devices, &device, property, &type,
			nullptr, 0, &requiredBytes);
		if (requiredBytes == 0) return {};
		std::vector<wchar_t> value(
			requiredBytes / sizeof(wchar_t) + 1, L'\0');
		DWORD actualBytes = requiredBytes;
		if (!SetupDiGetDeviceRegistryPropertyW(
			devices, &device, property, &type,
			reinterpret_cast<BYTE*>(value.data()),
			static_cast<DWORD>(value.size() * sizeof(wchar_t)),
			&actualBytes)) {
			return {};
		}
		if (type != REG_SZ && type != REG_MULTI_SZ) return {};
		const size_t available = (std::min)(
			value.size(), static_cast<size_t>(
				actualBytes / sizeof(wchar_t)));
		size_t length = 0;
		while (length < available && value[length] != L'\0') ++length;
		return std::wstring(value.data(), length);
	}

	std::wstring readDeviceRegistryString(HKEY key, const wchar_t* name) {
		DWORD type = 0;
		DWORD requiredBytes = 0;
		if (RegQueryValueExW(
			key, name, nullptr, &type, nullptr, &requiredBytes)
			!= ERROR_SUCCESS
			|| (type != REG_SZ && type != REG_EXPAND_SZ)
			|| requiredBytes == 0) {
			return {};
		}
		std::vector<wchar_t> value(
			requiredBytes / sizeof(wchar_t) + 1, L'\0');
		DWORD actualBytes = requiredBytes;
		if (RegQueryValueExW(
			key, name, nullptr, &type,
			reinterpret_cast<BYTE*>(value.data()), &actualBytes)
			!= ERROR_SUCCESS) {
			return {};
		}
		const size_t available = (std::min)(
			value.size(), static_cast<size_t>(
				actualBytes / sizeof(wchar_t)));
		size_t length = available;
		while (length > 0 && value[length - 1] == L'\0') --length;
		return std::wstring(value.data(), length);
	}

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
				m.portName = Utils::wideToUtf8(
					readDeviceRegistryString(hKey, L"PortName"));
				RegCloseKey(hKey);
			}
			if (m.portName.empty()) continue;

			// FriendlyName / DeviceDesc
			std::wstring description = readDeviceProperty(
				hDevInfo, devData, SPDRP_FRIENDLYNAME);
			if (description.empty()) {
				description = readDeviceProperty(
					hDevInfo, devData, SPDRP_DEVICEDESC);
			}
			m.description = Utils::wideToUtf8(description);

			// HardwareID から VID/PID
			parseVidPid(Utils::wideToUtf8(readDeviceProperty(
				hDevInfo, devData, SPDRP_HARDWAREID)), m.vid, m.pid);

			result.push_back(std::move(m));
		}

		SetupDiDestroyDeviceInfoList(hDevInfo);
		return result;
	}

} // namespace

namespace PortScanner {

	bool isArduinoVidPid(unsigned short vid, unsigned short pid) {
		// Arduino LLC / SRLはMega/ADK固有PIDだけを受け入れる。
		// Uno/Leonardo等を自動選択すると、署名照合で安全に停止できても
		// 利用者には「検出したのに書けない」という紛らわしい結果になる。
		// MEGA 2560 R3:        2341:0042
		// MEGA ADK R3:         2341:0044
		// MEGA 2560 (CDC):     2341:0010
		// MEGA ADK (旧版):      2341:003F
		// クローン互換:        1A86:7523 (CH340), 0403:6001 (FTDI), 10C4:EA60 (CP210x)
		switch (vid) {
		case 0x2341: // Arduino LLC
		case 0x2A03: // Arduino SRL
			return pid == 0x0010 || pid == 0x003F
				|| pid == 0x0042 || pid == 0x0044;
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
		// VID マッチを優先。ただし複数候補がある場合は勝手に先頭へ
		// 書き込まない。別のUSBシリアル機器をリセットする事故を防ぐ。
		std::vector<std::string> vidMatches;
		for (const auto& d : devs) {
			if (isArduinoVidPid(d.vid, d.pid)) vidMatches.push_back(d.portName);
		}
		std::sort(vidMatches.begin(), vidMatches.end());
		vidMatches.erase(std::unique(vidMatches.begin(), vidMatches.end()),
			vidMatches.end());
		if (vidMatches.size() == 1) return vidMatches.front();
		if (vidMatches.size() > 1) return "";

		// フォールバック: ボード名が明示的にMega/ADKを示すもの。
		std::vector<std::string> descriptionMatches;
		for (const auto& d : devs) {
			std::string lower = d.description;
			std::transform(lower.begin(), lower.end(), lower.begin(),
				[](unsigned char c) {
					return static_cast<char>(std::tolower(c));
				});
			if (lower.find("mega") != std::string::npos
				|| lower.find("adk") != std::string::npos) {
				descriptionMatches.push_back(d.portName);
			}
		}
		std::sort(descriptionMatches.begin(), descriptionMatches.end());
		descriptionMatches.erase(
			std::unique(descriptionMatches.begin(), descriptionMatches.end()),
			descriptionMatches.end());
		if (descriptionMatches.size() == 1) return descriptionMatches.front();
		if (descriptionMatches.size() > 1) return "";

		// VIDを取得できない機器でも、接続中COMポートが1つだけなら選べる。
		// ただし公式VIDの非Mega機は既知の非対応ボードなので自動選択しない。
		const bool knownUnsupportedOfficialBoard = std::any_of(
			devs.begin(), devs.end(), [](const DeviceMatch& device) {
				return (device.vid == 0x2341 || device.vid == 0x2A03)
					&& !PortScanner::isArduinoVidPid(
						device.vid, device.pid);
			});
		auto names = readSerialCommRegistry();
		if (names.size() == 1 && !knownUnsupportedOfficialBoard) {
			return names.front();
		}
		return "";
	}

} // namespace PortScanner
