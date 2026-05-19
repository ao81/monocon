#include "ra4m1_uploader.h"
#include "utils.h"
#include "port_scanner.h"

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>

#include <chrono>
#include <filesystem>
#include <sstream>
#include <thread>
#include <algorithm>
#include <cstdio>

#pragma comment(lib, "setupapi.lib")

namespace {

	// Uno R4 WiFi 関連の USB VID / PID
	constexpr unsigned short kArduinoVid = 0x2341;
	constexpr unsigned short kAppPid     = 0x1002; // アプリモード CDC
	constexpr unsigned short kDfuPid     = 0x006D; // DFU/Bootloader モード

	// RA4M1 のユーザーフラッシュ開始アドレス
	// (0x0000-0x3FFF は Arduino DFU ブートローダ領域)
	constexpr uint32_t kUserFlashStart = 0x4000;

	double elapsedMs(std::chrono::steady_clock::time_point t0) {
		return std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - t0).count();
	}

	// =========================================================================
	// SerialPort RAII: 1200bps タッチ専用の最小ラッパ
	// 旧 STK500v2 と違って書き込みすら行わない。open → close するだけで
	// MCU 内のアプリが DFU 遷移コードを実行する。
	// =========================================================================
	class TouchSerial {
	public:
		HANDLE h = INVALID_HANDLE_VALUE;
		~TouchSerial() { close(); }

		// 1200bps で open。DTR を一度トグルして DTR エッジ駆動の R4 ボードでも
		// 確実にタッチ判定させる。
		bool openAt1200(const std::string& port) {
			std::string dev = "\\\\.\\" + port;
			h = CreateFileA(dev.c_str(),
				GENERIC_READ | GENERIC_WRITE, 0, nullptr,
				OPEN_EXISTING, 0, nullptr);
			if (h == INVALID_HANDLE_VALUE) return false;

			DCB dcb{};
			dcb.DCBlength = sizeof(dcb);
			if (!GetCommState(h, &dcb)) { close(); return false; }
			dcb.BaudRate = 1200;
			dcb.ByteSize = 8;
			dcb.Parity   = NOPARITY;
			dcb.StopBits = ONESTOPBIT;
			dcb.fBinary  = TRUE;
			dcb.fParity  = FALSE;
			dcb.fOutxCtsFlow = FALSE;
			dcb.fOutxDsrFlow = FALSE;
			dcb.fDtrControl  = DTR_CONTROL_DISABLE; // 一旦 LOW
			dcb.fRtsControl  = RTS_CONTROL_DISABLE;
			dcb.fOutX = FALSE;
			dcb.fInX  = FALSE;
			dcb.fNull = FALSE;
			dcb.fAbortOnError = FALSE;
			if (!SetCommState(h, &dcb)) { close(); return false; }

			// DTR を一旦立ててから落とす (HIGH→LOW エッジを作る)。
			// Arduino UNO R4 のアプリ側は USB CDC line coding 1200bps の検出と
			// DTR LOW のどちらでも DFU 遷移ハンドラを起動するが、両方やる方が確実。
			EscapeCommFunction(h, SETDTR);
			Sleep(2);
			EscapeCommFunction(h, CLRDTR);

			return true;
		}

		void close() {
			if (h != INVALID_HANDLE_VALUE) {
				CloseHandle(h);
				h = INVALID_HANDLE_VALUE;
			}
		}
	};

	// アプリポートが SERIALCOMM から消えるまで待つ。
	// 典型は 1200bps タッチから 200-700ms。最大 3 秒で打ち切る。
	bool waitPortGone(const std::string& appPort, int maxMs) {
		using namespace std::chrono;
		auto deadline = steady_clock::now() + milliseconds(maxMs);
		while (steady_clock::now() < deadline) {
			auto names = PortScanner::listNames();
			bool present = false;
			for (const auto& n : names) {
				if (n == appPort) { present = true; break; }
			}
			if (!present) return true;
			std::this_thread::sleep_for(milliseconds(20));
		}
		return false;
	}

	// DFU クラスデバイス (USB DFU: クラス 0xFE / サブクラス 0x01) として
	// 2341:006D が出現するのを待つ。
	// SetupAPI ではクラス GUID = GUID_DEVCLASS_USB を使う。
	//
	// 簡易実装: HardwareID 内の "VID_2341&PID_006D" 部分文字列マッチで判定。
	bool waitDfuDevice(int maxMs) {
		using namespace std::chrono;
		auto deadline = steady_clock::now() + milliseconds(maxMs);
		const char* magic = "VID_2341&PID_006D";

		while (steady_clock::now() < deadline) {
			HDEVINFO hDevInfo = SetupDiGetClassDevsA(
				nullptr, "USB", nullptr,
				DIGCF_PRESENT | DIGCF_ALLCLASSES);
			if (hDevInfo != INVALID_HANDLE_VALUE) {
				SP_DEVINFO_DATA devData{};
				devData.cbSize = sizeof(devData);
				for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devData); ++i) {
					char hwid[1024] = { 0 };
					if (SetupDiGetDeviceRegistryPropertyA(hDevInfo, &devData,
						SPDRP_HARDWAREID, nullptr, reinterpret_cast<BYTE*>(hwid),
						sizeof(hwid), nullptr)) {
						// 大文字小文字を許容するため atoi 風に小文字化したコピーで比較
						std::string s(hwid);
						std::transform(s.begin(), s.end(), s.begin(),
							[](unsigned char c) { return (char)std::toupper(c); });
						if (s.find(magic) != std::string::npos) {
							SetupDiDestroyDeviceInfoList(hDevInfo);
							return true;
						}
					}
				}
				SetupDiDestroyDeviceInfoList(hDevInfo);
			}
			std::this_thread::sleep_for(milliseconds(30));
		}
		return false;
	}

} // namespace

namespace Ra4m1Uploader {

	// =========================================================================
	// arm-none-eabi-objcopy で .elf → .bin
	// =========================================================================
	bool generateBin(const std::string& objcopyExe,
		const std::string& elfPath,
		const std::string& binPath,
		std::string& err) {
		std::string args = "-O binary \"" + elfPath + "\" \"" + binPath + "\"";
		auto pr = runProcess(objcopyExe, args, "", true);
		if (pr.exitCode != 0) {
			err = "arm-none-eabi-objcopy -O binary failed:\n" + pr.error + pr.output;
			return false;
		}
		return true;
	}

	// =========================================================================
	// アップロード本体
	// =========================================================================
	UploadStats uploadUnoR4Wifi(const std::string& appPort,
		const std::string& binPath,
		const std::string& dfuUtilExe) {
		UploadStats stats;
		stats.appPort = appPort;
		auto t0 = std::chrono::steady_clock::now();

		// 0) 入口チェック
		if (!Utils::fileExists(binPath)) {
			stats.errorMessage = "bin file not found: " + binPath;
			return stats;
		}
		if (!Utils::fileExists(dfuUtilExe)) {
			stats.errorMessage = "dfu-util.exe not found: " + dfuUtilExe;
			return stats;
		}

		// 既に DFU モードに居る可能性: その場合は 1200bps タッチは飛ばす。
		// (再書き込み時 / アップロード失敗後の復旧パス)
		bool alreadyInDfu = waitDfuDevice(50);

		if (!alreadyInDfu) {
			// 1) 1200bps タッチで DFU モード起動
			if (appPort.empty()) {
				stats.errorMessage = "No application COM port to touch "
					"(plug in UNO R4 WiFi or pass --port)";
				return stats;
			}
			auto tTouch = std::chrono::steady_clock::now();
			{
				TouchSerial ts;
				if (!ts.openAt1200(appPort)) {
					stats.errorMessage = "Cannot open " + appPort + " at 1200bps";
					return stats;
				}
				// open とほぼ同時に DCB 設定で line coding が 1200 になる。
				// 短い settle のあと close (= DTR HIGH に戻し、デバイスが切断を検出)。
				Sleep(2);
			}
			stats.touchMs = elapsedMs(tTouch);

			// 2) アプリ COM の消失を待つ
			auto tGone = std::chrono::steady_clock::now();
			if (!waitPortGone(appPort, 3000)) {
				// 消えない場合でも続行 (一部の R4 派生ボードではアプリ CDC が
				// 別の COM 番号でそのまま残ることがある)
			}
			stats.waitPortGoneMs = elapsedMs(tGone);

			// 3) DFU デバイス出現を待つ (最大 5 秒)
			auto tDfuWait = std::chrono::steady_clock::now();
			if (!waitDfuDevice(5000)) {
				stats.errorMessage = "DFU device 2341:006D not detected after 1200bps touch. "
					"Confirm WinUSB driver is installed for UNO R4 WiFi DFU.";
				return stats;
			}
			stats.waitDfuMs = elapsedMs(tDfuWait);
		}
		stats.bootloaderId = "2341:006D";

		// 4) dfu-util で書き込み
		// 引数:
		//   -d 2341:006D      : 対象デバイス指定 (複数 R4 を区別)
		//   -a 0              : alt-setting 0 (RA4M1 メインフラッシュ)
		//   -s 0x4000:leave   : 0x4000 から書き込み、書き込み後にアプリへ復帰
		//   -D <bin>          : ダウンロード (PC→デバイス)
		auto tDfu = std::chrono::steady_clock::now();
		std::ostringstream args;
		args << "-d " << "2341:006D"
			<< " -a 0"
			<< " -s 0x" << std::hex << kUserFlashStart << std::dec << ":leave"
			<< " -D \"" << binPath << "\"";

		auto pr = runProcess(dfuUtilExe, args.str(), "", true);
		stats.dfuMs = elapsedMs(tDfu);
		stats.dfuOutput = pr.output + pr.error;

		if (pr.exitCode != 0) {
			stats.errorMessage = "dfu-util failed (exit " + std::to_string(pr.exitCode) + ")";
			return stats;
		}

		// 書き込みサイズ
		std::error_code ec;
		auto sz = std::filesystem::file_size(binPath, ec);
		if (!ec) stats.bytesWritten = (size_t)sz;

		stats.totalMs = elapsedMs(t0);
		stats.success = true;
		return stats;
	}

} // namespace Ra4m1Uploader
