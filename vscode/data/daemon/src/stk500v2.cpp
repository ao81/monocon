#include "stk500v2.h"

#include <windows.h>
#include <chrono>
#include <fstream>
#include <sstream>

namespace {

	// === STK500v2 プロトコル定数 ===
	constexpr uint8_t MESSAGE_START = 0x1B;
	constexpr uint8_t TOKEN = 0x0E;

	constexpr uint8_t CMD_SIGN_ON = 0x01;
	constexpr uint8_t CMD_LOAD_ADDRESS = 0x06;
	constexpr uint8_t CMD_ENTER_PROGMODE_ISP = 0x10;
	constexpr uint8_t CMD_LEAVE_PROGMODE_ISP = 0x11;
	constexpr uint8_t CMD_PROGRAM_FLASH_ISP = 0x13;

	constexpr uint8_t STATUS_CMD_OK = 0x00;

	// === ATmega2560 ===
	constexpr size_t MEGA2560_PAGE_SIZE = 256;       // bytes
	constexpr size_t MEGA2560_FLASH_SIZE = 256 * 1024; // 256KB

	double elapsedMs(std::chrono::steady_clock::time_point t0) {
		return std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - t0).count();
	}

	// =========================================================================
	// SerialPort: シリアルポートのRAIIラッパ
	// =========================================================================
	class SerialPort {
	public:
		HANDLE h = INVALID_HANDLE_VALUE;

		~SerialPort() { close(); }

		bool open(const std::string& port) {
			std::string dev = "\\\\.\\" + port;
			h = CreateFileA(dev.c_str(),
				GENERIC_READ | GENERIC_WRITE, 0, nullptr,
				OPEN_EXISTING, 0, nullptr);
			if (h == INVALID_HANDLE_VALUE) return false;

			// ドライバ側のバッファを大きく確保: フルページ (270B) を 1 USB 転送で受け切れる。
			// 既定値はドライバ依存 (FTDI=4096 など) だが、小さい場合でも上書きで最適化。
			SetupComm(h, 4096, 4096);

			DCB dcb{};
			dcb.DCBlength = sizeof(dcb);
			if (!GetCommState(h, &dcb)) { close(); return false; }
			dcb.BaudRate = 115200;
			dcb.ByteSize = 8;
			dcb.Parity = NOPARITY;
			dcb.StopBits = ONESTOPBIT;
			dcb.fBinary = TRUE;
			dcb.fParity = FALSE;
			dcb.fOutxCtsFlow = FALSE;
			dcb.fOutxDsrFlow = FALSE;
			dcb.fDtrControl = DTR_CONTROL_ENABLE;
			dcb.fRtsControl = RTS_CONTROL_ENABLE;
			// フロー制御を完全に無効化
			dcb.fOutX = FALSE;
			dcb.fInX = FALSE;
			dcb.fNull = FALSE;
			dcb.fAbortOnError = FALSE;
			dcb.fErrorChar = FALSE;
			if (!SetCommState(h, &dcb)) { close(); return false; }

			// 「データ完了待ち」型のタイムアウト。
			// ReadIntervalTimeout=MAXDWORD と ReadTotalTimeoutMultiplier=0 にすると、
			// ReadFile は「指定バイト数を全部受信」or「ReadTotalTimeoutConstant 経過」
			// のどちらかで戻る。USB-Serial のバイト間ギャップで取りこぼさない。
			setReadTimeout(200);
			return true;
		}

		// 動的にタイムアウトを切り替える。sync 中は短く、ページ書込中は長め。
		void setReadTimeout(DWORD ms) {
			COMMTIMEOUTS to{};
			to.ReadIntervalTimeout = MAXDWORD;
			to.ReadTotalTimeoutConstant = ms;
			to.ReadTotalTimeoutMultiplier = 0;
			to.WriteTotalTimeoutConstant = ms;
			to.WriteTotalTimeoutMultiplier = 0;
			SetCommTimeouts(h, &to);
		}

		void close() {
			if (h != INVALID_HANDLE_VALUE) {
				CloseHandle(h);
				h = INVALID_HANDLE_VALUE;
			}
		}

		// Mega 2560 のリセット: DTR をトグルしてブートローダ起動
		// Mega 2560 のリセット回路はコンデンサ結合 (DTR──[100nF]──RESET, RESET 側 10kΩ プルアップ) なので、
		// DTR の HIGH→LOW→HIGH エッジでだけリセットがかかる。
		// ずっと LOW でも HIGH でもリセットしない。
		// RC 時定数 = 10kΩ × 100nF = 1ms。実際のリセットパルス幅は約 1ms。
		// よって長時間 LOW を維持しても効果は無く、純粋なドライバ反映待ちで十分。
		void toggleReset() {
			// HIGH を確実に立てるための最低限の settle (旧: 50ms → 2ms)
			EscapeCommFunction(h, SETDTR);
			Sleep(2);

			// HIGH → LOW: コンデンサ経由で RESET ピンに負パルス → MCU リセット
			// 旧 100ms はドライバ反映と一部 USB-Serial の遅延を吸収するための余裕。
			// 20ms あれば CH340/FTDI/16U2 すべて反映済み。RC 1ms に対して十分過大。
			EscapeCommFunction(h, CLRDTR);
			Sleep(20);

			// LOW → HIGH: リセット解除 → ブートローダ起動
			EscapeCommFunction(h, SETDTR);

			// リセット直後のゴミデータを捨てる
			PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
		}

		bool write(const uint8_t* data, size_t n) {
			DWORD written = 0;
			return WriteFile(h, data, (DWORD)n, &written, nullptr) && written == n;
		}

		bool read(uint8_t* buf, size_t n) {
			// ReadIntervalTimeout=MAXDWORD のため、ReadFile は
			// 「指定バイト数全部受信」or「ReadTotalTimeoutConstant 経過」で戻る。
			// 1 回の ReadFile が 500ms タイムアウト。最大 4 回 = 2 秒まで待つ。
			constexpr int kMaxLoops = 4;
			DWORD total = 0;
			for (int loop = 0; loop < kMaxLoops && total < n; ++loop) {
				DWORD r = 0;
				if (!ReadFile(h, buf + total, (DWORD)(n - total), &r, nullptr)) return false;
				total += r;
			}
			return total == n;
		}
	};

	// =========================================================================
	// STK500v2 メッセージ送受信
	// =========================================================================
	class Stk500v2Client {
	public:
		SerialPort& port;
		uint8_t seq = 1;

		explicit Stk500v2Client(SerialPort& p) : port(p) {}

		bool sendMessage(const std::vector<uint8_t>& body) {
			std::vector<uint8_t> msg;
			msg.reserve(6 + body.size());
			msg.push_back(MESSAGE_START);
			msg.push_back(seq);
			msg.push_back((uint8_t)((body.size() >> 8) & 0xFF));
			msg.push_back((uint8_t)(body.size() & 0xFF));
			msg.push_back(TOKEN);
			msg.insert(msg.end(), body.begin(), body.end());
			uint8_t chk = 0;
			for (uint8_t b : msg) chk ^= b;
			msg.push_back(chk);
			return port.write(msg.data(), msg.size());
		}

		bool recvMessage(std::vector<uint8_t>& body) {
			// PurgeComm 後 / 直前送信に対する応答であれば、RX バッファ先頭は確実に
			// MESSAGE_START になる。ヘッダ 5 バイトを 1 ReadFile で取得して
			// シスコール回数を 1 つ削る (ページ毎に 1 回 → 数十 ms オーダー)。
			uint8_t hdr5[5];
			if (!port.read(hdr5, 5)) return false;

			if (hdr5[0] != MESSAGE_START) {
				// 異常系: 古いゴミが先頭にある場合は線形に MESSAGE_START を探す
				// (1 KB 以内に必ず正しい応答が来るとみなす)
				int searched = 0;
				while (hdr5[0] != MESSAGE_START) {
					if (++searched > 1024) return false;
					uint8_t b;
					if (!port.read(&b, 1)) return false;
					hdr5[0] = b;
				}
				// 残り 4 バイトを再度読む
				if (!port.read(hdr5 + 1, 4)) return false;
			}

			if (hdr5[4] != TOKEN) return false;

			uint16_t sz = ((uint16_t)hdr5[2] << 8) | hdr5[3];
			body.resize(sz);
			if (sz > 0 && !port.read(body.data(), sz)) return false;

			uint8_t chk;
			if (!port.read(&chk, 1)) return false;

			uint8_t calc = MESSAGE_START ^ hdr5[1] ^ hdr5[2] ^ hdr5[3] ^ hdr5[4];
			for (uint8_t bb : body) calc ^= bb;
			if (calc != chk) return false;

			seq++;
			return true;
		}

		bool exchange(const std::vector<uint8_t>& req, std::vector<uint8_t>& resp) {
			if (!sendMessage(req)) return false;
			return recvMessage(resp);
		}

		bool signOn() {
			std::vector<uint8_t> resp;
			if (!sendMessage({ CMD_SIGN_ON })) return false;
			if (!recvMessage(resp)) return false;
			return resp.size() >= 2 && resp[0] == CMD_SIGN_ON && resp[1] == STATUS_CMD_OK;
		}

		bool enterProgMode() {
			// ATmega2560 の標準 ISP 入場シーケンス
			std::vector<uint8_t> req = {
				CMD_ENTER_PROGMODE_ISP,
				200, 100, 25, 32, 0,
				0x53, 3,
				0xAC, 0x53, 0x00, 0x00
			};
			std::vector<uint8_t> resp;
			if (!exchange(req, resp)) return false;
			return resp.size() >= 2 && resp[0] == CMD_ENTER_PROGMODE_ISP && resp[1] == STATUS_CMD_OK;
		}

		bool leaveProgMode() {
			std::vector<uint8_t> req = { CMD_LEAVE_PROGMODE_ISP, 1, 1 };
			std::vector<uint8_t> resp;
			if (!exchange(req, resp)) return false;
			return resp.size() >= 2 && resp[0] == CMD_LEAVE_PROGMODE_ISP && resp[1] == STATUS_CMD_OK;
		}

		// Mega 2560 (>64KB) は bit31=1 で拡張アドレス指定。アドレスは「ワード」単位。
		bool loadAddress(uint32_t wordAddr) {
			std::vector<uint8_t> req = {
				CMD_LOAD_ADDRESS,
				(uint8_t)(((wordAddr >> 24) & 0xFF) | 0x80),
				(uint8_t)((wordAddr >> 16) & 0xFF),
				(uint8_t)((wordAddr >> 8) & 0xFF),
				(uint8_t)(wordAddr & 0xFF)
			};
			std::vector<uint8_t> resp;
			if (!exchange(req, resp)) return false;
			return resp.size() >= 2 && resp[0] == CMD_LOAD_ADDRESS && resp[1] == STATUS_CMD_OK;
		}

		bool programPage(const uint8_t* data, size_t len) {
			// ホットパス: ヒープを使わずスタック上で 1 メッセージを組み立てて 1 回の WriteFile で送る。
			// レイアウト:
			//   [0] MESSAGE_START
			//   [1] seq
			//   [2..3] body size (BE)
			//   [4] TOKEN
			//   [5..14] CMD_PROGRAM_FLASH_ISP + 9 byte param header
			//   [15..15+len-1] flash data
			//   [15+len] checksum (XOR)
			// 合計サイズは 16 + len。Mega2560 は len=256 で 272 byte。
			constexpr size_t kHeaderBytes = 5 + 10; // フレーム 5 + cmd ヘッダ 10
			uint8_t buf[16 + MEGA2560_PAGE_SIZE];   // 最大ページサイズ固定で十分
			if (len > MEGA2560_PAGE_SIZE) return false;

			const uint16_t bodySize = (uint16_t)(10 + len);
			buf[0] = MESSAGE_START;
			buf[1] = seq;
			buf[2] = (uint8_t)((bodySize >> 8) & 0xFF);
			buf[3] = (uint8_t)(bodySize & 0xFF);
			buf[4] = TOKEN;

			buf[5] = CMD_PROGRAM_FLASH_ISP;
			buf[6] = (uint8_t)((len >> 8) & 0xFF);
			buf[7] = (uint8_t)(len & 0xFF);
			buf[8] = 0xC1;          // mode: paged + write page
			buf[9] = 10;            // delay
			buf[10] = 0x40;         // cmd1
			buf[11] = 0x4C;         // cmd2 (write page)
			buf[12] = 0x20;         // cmd3 (read for value-poll)
			buf[13] = 0x00;         // poll1
			buf[14] = 0x00;         // poll2

			memcpy(buf + kHeaderBytes, data, len);

			uint8_t chk = 0;
			const size_t endIdx = kHeaderBytes + len;
			for (size_t i = 0; i < endIdx; ++i) chk ^= buf[i];
			buf[endIdx] = chk;

			if (!port.write(buf, endIdx + 1)) return false;

			std::vector<uint8_t> resp;
			if (!recvMessage(resp)) return false;
			return resp.size() >= 2 && resp[0] == CMD_PROGRAM_FLASH_ISP && resp[1] == STATUS_CMD_OK;
		}
	};

} // anonymous namespace

namespace Stk500v2 {

	// =========================================================================
	// Intel HEX パーサ
	// =========================================================================
	bool readIntelHex(const std::string& path, std::vector<uint8_t>& flash,
		std::string& err) {
		std::ifstream f(path);
		if (!f) { err = "Cannot open hex: " + path; return false; }

		flash.assign(MEGA2560_FLASH_SIZE, 0xFF);
		uint32_t baseAddr = 0;
		size_t maxAddr = 0;
		std::string line;

		auto hex2 = [](char a, char b) -> int {
			auto v = [](char c) -> int {
				if (c >= '0' && c <= '9') return c - '0';
				if (c >= 'A' && c <= 'F') return c - 'A' + 10;
				if (c >= 'a' && c <= 'f') return c - 'a' + 10;
				return -1;
				};
			int hi = v(a), lo = v(b);
			if (hi < 0 || lo < 0) return -1;
			return (hi << 4) | lo;
			};

		while (std::getline(f, line)) {
			if (!line.empty() && line.back() == '\r') line.pop_back();
			if (line.empty() || line[0] != ':') continue;
			if (line.size() < 11) { err = "Bad hex line"; return false; }

			int cnt = hex2(line[1], line[2]);
			int aH = hex2(line[3], line[4]);
			int aL = hex2(line[5], line[6]);
			int type = hex2(line[7], line[8]);
			if (cnt < 0 || aH < 0 || aL < 0 || type < 0) {
				err = "Bad hex bytes"; return false;
			}
			uint32_t addr = ((uint32_t)aH << 8) | (uint32_t)aL;

			if (type == 0x00) {
				uint32_t a = baseAddr + addr;
				if (a + cnt > flash.size()) {
					err = "Hex address out of flash"; return false;
				}
				for (int i = 0; i < cnt; ++i) {
					int b = hex2(line[9 + i * 2], line[10 + i * 2]);
					if (b < 0) { err = "Bad data byte"; return false; }
					flash[a + i] = (uint8_t)b;
				}
				if (a + cnt > maxAddr) maxAddr = a + cnt;
			} else if (type == 0x01) {
				break; // EOF
			} else if (type == 0x02) {
				int hH = hex2(line[9], line[10]);
				int hL = hex2(line[11], line[12]);
				if (hH < 0 || hL < 0) { err = "Bad seg"; return false; }
				baseAddr = (((uint32_t)hH << 8) | (uint32_t)hL) << 4;
			} else if (type == 0x04) {
				int hH = hex2(line[9], line[10]);
				int hL = hex2(line[11], line[12]);
				if (hH < 0 || hL < 0) { err = "Bad lin"; return false; }
				baseAddr = (((uint32_t)hH << 8) | (uint32_t)hL) << 16;
			}
		}

		// ページ境界に切り上げ
		size_t pages = (maxAddr + MEGA2560_PAGE_SIZE - 1) / MEGA2560_PAGE_SIZE;
		flash.resize(pages * MEGA2560_PAGE_SIZE, 0xFF);
		return true;
	}

	// =========================================================================
	// アップロード本体
	// =========================================================================
	UploadStats uploadMega2560(const std::string& port,
		const std::vector<uint8_t>& flash) {
		UploadStats stats;
		auto t0 = std::chrono::steady_clock::now();

		// 1) シリアルポートを開く
		SerialPort sp;
		if (!sp.open(port)) {
			stats.errorMessage = "Cannot open " + port;
			return stats;
		}
		stats.openMs = elapsedMs(t0);
		auto t1 = std::chrono::steady_clock::now();

		// 2) MCU リセット (DTR トグルでブートローダ起動)
		sp.toggleReset();
		stats.resetMs = elapsedMs(t1);
		auto t2 = std::chrono::steady_clock::now();

		// Mega 2560 wiring ブートローダの起動シーケンス:
		// - リセット解除直後、ブートローダの初期化 (UART 設定, ウォッチドッグ解除等) に ~50ms
		// - 旧来の Sleep(250) は最悪値マージン込みの過剰待機。
		// - 80ms 確定待機 → 50ms タイムアウトでアダプティブ sync すれば、
		//   ブートローダ準備完了の瞬間に sync 成立する。
		Sleep(80);

		// 3) ブートローダと sync: 短タイムアウト + 早期再試行
		// 全体予算を 1.2 秒で打ち切る (旧: 6 retries × 500ms = 最悪 3 秒)
		sp.setReadTimeout(50);

		Stk500v2Client cli(sp);
		bool synced = false;
		int sendFailures = 0;
		int recvFailures = 0;
		int badResponses = 0;

		auto syncStart = std::chrono::steady_clock::now();
		while (true) {
			auto syncElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - syncStart).count();
			if (syncElapsed > 1200) break;

			std::vector<uint8_t> resp;
			if (!cli.sendMessage({ CMD_SIGN_ON })) {
				sendFailures++;
				continue;
			}
			if (!cli.recvMessage(resp)) {
				recvFailures++;
				continue;
			}
			if (resp.size() >= 2 && resp[0] == CMD_SIGN_ON && resp[1] == STATUS_CMD_OK) {
				synced = true;
				break;
			}
			badResponses++;
		}

		// プログラミング中は通常タイムアウトに戻す。
		// (ページ書込中はフラッシュ erase+write で ~9ms 停止するため、最低でも数十 ms 必要)
		sp.setReadTimeout(200);

		if (!synced) {
			std::ostringstream diag;
			diag << "Sync failed. send_fails=" << sendFailures
				<< " recv_fails=" << recvFailures
				<< " bad_resp=" << badResponses;
			stats.errorMessage = diag.str();
			return stats;
		}
		if (!cli.enterProgMode()) {
			stats.errorMessage = "Failed to enter programming mode";
			return stats;
		}
		stats.syncMs = elapsedMs(t2);
		auto t3 = std::chrono::steady_clock::now();

		// 4) 全ページ書き込み (0xFF だけのページはスキップ)
		// 重要: Wiring ブートローダ (公式 Mega 2560) は CMD_PROGRAM_FLASH_ISP の後、
		//   内部の `address` 変数を「書き込んだバイト数 / 2」だけ自動的に進める。
		//   よって連続するページに対して loadAddress を毎回呼ぶ必要はない。
		//   呼ぶのは「直前のページから連続でない」場合のみ (= 初回 or 0xFF スキップ後)。
		// この削減により ~3-5 ms × ページ数 を削れる (32 ページなら ~100ms)。
		size_t pages = flash.size() / MEGA2560_PAGE_SIZE;
		uint32_t expectedNextWordAddr = ~0u;  // 「未確定」を表すセンチネル
		for (size_t p = 0; p < pages; ++p) {
			size_t addr = p * MEGA2560_PAGE_SIZE;

			// 64bit 単位で 0xFF 判定 (バイト単位より高速)
			bool allFF = true;
			const uint64_t kFFMask = 0xFFFFFFFFFFFFFFFFull;
			const uint64_t* p64 = reinterpret_cast<const uint64_t*>(&flash[addr]);
			for (size_t i = 0; i < MEGA2560_PAGE_SIZE / sizeof(uint64_t); ++i) {
				if (p64[i] != kFFMask) { allFF = false; break; }
			}
			if (allFF) continue;

			uint32_t wordAddr = (uint32_t)(addr / 2);
			if (wordAddr != expectedNextWordAddr) {
				if (!cli.loadAddress(wordAddr)) {
					stats.errorMessage = "loadAddress failed at page " + std::to_string(p);
					return stats;
				}
			}
			if (!cli.programPage(&flash[addr], MEGA2560_PAGE_SIZE)) {
				stats.errorMessage = "programPage failed at page " + std::to_string(p);
				return stats;
			}
			// ブートローダ側のアドレスはページサイズ / 2 だけ進んだはず
			expectedNextWordAddr = wordAddr + (uint32_t)(MEGA2560_PAGE_SIZE / 2);
			stats.bytesWritten += MEGA2560_PAGE_SIZE;
			stats.pagesWritten++;
		}
		stats.progMs = elapsedMs(t3);
		auto t4 = std::chrono::steady_clock::now();

		// 5) プログラミングモード退出
		cli.leaveProgMode();
		stats.leaveMs = elapsedMs(t4);

		stats.totalMs = elapsedMs(t0);
		stats.success = true;
		return stats;
	}

} // namespace Stk500v2