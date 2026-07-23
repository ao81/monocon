///////////////////////////////////////////////////////////////////////////////
//
// hardTest2026 --- 制御対象回路の動作テスト（mono_con.h 2026年版用）
//
// 7セグ（左桁）---デバイス番号（1～3）
//                タクトスイッチをダブルクリックするごとに 1→2→3→1→… と変化
//                1：FullColor LED
//                2：Stepping Motor
//                3：DC Motor
// 7セグ（右桁）---動作番号（0～）
//                タクトスイッチをクリックするごとに，各デバイスに応じて
//                0→1→2→… とサイクリックに変化し，数字に応じた動作をする
// 長押し（1秒）---初期状態（デバイス1・動作0）に戻り，全デバイス停止
//
// クリック判定の仕様
//   ・押下から500ms以内に「離す→再押下」      → ダブルクリック
//   ・押下から500ms以内に離し，再押下なし     → シングルクリック（500ms経過時に確定）
//   ・500ms以上押し続け，1000ms未満で離す     → シングルクリック（離した時点で確定）
//   ・1000ms押し続ける                        → 長押し
//
// 旧版からの主な変更点
//   ・USE_TIMER3_ISR → USE_TIMER3（新ヘッダーのマクロ名に合わせた）
//   ・led_stepmotor クラスの lm.led()/lm.spm()/lm.flush() を使用
//   ・相管理変数 ex を廃止し，クラス内の CW/CCW に任せた
//   ・ISR共有変数に volatile を付加，16bit変数は ATOMIC_BLOCK で保護
//   ・デバイス切替時に LED消灯・モータ励磁OFF・DCモータ停止を実行
//   ・KeySense() の戻り値を enum 化し，default を追加
//   ・kFlag を削除（KeySense() はイベントを1回しか返さないため不要）
//   ・長押し検出（KEY_LONG）を追加
//
///////////////////////////////////////////////////////////////////////////////
#define USE_TIMER3
#include "mono_con.h"
#include <util/atomic.h>

#define COUNT_OF(a) (int)(sizeof(a) / sizeof((a)[0]))

///// クリック判定の時間設定 /////
#define T_DBLCLICK  250   // ダブルクリック判定窓 [ms]
#define T_LONGPRESS 1000  // 長押し判定時間 [ms]

///// KeySense() の戻り値 /////
enum {
	KEY_BUSY = -1,  // 判定中
	KEY_NONE = 0,   // 入力なし
	KEY_SINGLE = 1,   // シングルクリック
	KEY_DOUBLE = 2,   // ダブルクリック
	KEY_LONG = 3    // 長押し
};

///// ISRと共有する変数（必ずvolatile） /////
volatile boolean tsw = HIGH;  // タクトスイッチのサンプリング値
volatile word tc = 0;         // 汎用msカウンタ（ステッピングモータ周期用）
volatile int kTime = 0;       // クリック判定タイマ [ms]
volatile int toneTime = 0;    // ブザー鳴動残り時間 [ms]

///// loop内だけで使う変数 /////
int kst = 0;              // KeySense() の状態
boolean tswFlag = false;  // 1回目の押下が離されたかの判定用
int devNo = 1;            // デバイス番号（7セグ左桁）
int value = 0;            // 動作番号（7セグ右桁）

///// 各デバイス動作を定義したデータ /////
int ledPara[] = { B000, B001, B010, B100, B011, B110, B101, B111 };  // FullColor LED 点灯データ(GBR)
int smPara[] = { -1, 50, 20, 10, -1, -50, -20, -10 };                // Stepping motor 回転データ（単位はms/step，+:CW，-:CCW）
int dmPara[] = { 0, 70, 120, 170, 0, -70, -100, -170 };              // DC motor 回転データ（PWM値，0:停止，+:CW，-:CCW）

///////////////////////////////////////////////////////////////////////////////
// DCモータ制御（speed: 0=ブレーキ，+:CW，-:CCW）
///////////////////////////////////////////////////////////////////////////////
void DMset(int speed) {
	if (speed == 0) {
		digitalWrite(FIN_PIN, HIGH);  // 両方HIGH＝ブレーキ
		digitalWrite(RIN_PIN, HIGH);
	} else if (speed > 0) {
		digitalWrite(RIN_PIN, LOW);
		analogWrite(FIN_PIN, speed);
	} else {
		digitalWrite(FIN_PIN, LOW);
		analogWrite(RIN_PIN, -speed);
	}
}

///////////////////////////////////////////////////////////////////////////////
void setup(void) {
	config_init();
	serial_init();
	noTone(BZ_PIN);

	tsw = digitalRead(BOARD_SW_PIN);  // タイマ割込みでサンプリングするので，最初の値をsetup()内で読んでおく
	devNo = 1;
	value = 0;
	disp(num[devNo], num[value]);
	lm.led(0).spm(STOP).flush();  // LED消灯・モータ励磁OFF
}

///////////////////////////////////////////////////////////////////////////////
// Timer3 1ms割込み
///////////////////////////////////////////////////////////////////////////////
ISR(TIMER3_COMPA_vect) {
	static byte sTime = 0;

	///// タクトスイッチのサンプリング（5ms周期） /////
	if (++sTime >= 5) {
		sTime = 0;
		tsw = digitalRead(BOARD_SW_PIN);
	}

	tc++;

	///// クリック判定処理用 /////
	if (kTime > 0) kTime--;

	///// ブザー処理用（0になった瞬間だけnoToneを呼ぶ） /////
	if (toneTime > 0) {
		if (--toneTime == 0) noTone(BZ_PIN);
	}
}

///////////////////////////////////////////////////////////////////////////////
// ブザーを200ms鳴らす
///////////////////////////////////////////////////////////////////////////////
void beep(unsigned int freq) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { toneTime = 200; }
	tone(BZ_PIN, freq);
}

///////////////////////////////////////////////////////////////////////////////
// 全デバイスを停止状態にする
///////////////////////////////////////////////////////////////////////////////
void allStop(void) {
	lm.led(0).spm(STOP).flush();  // LED消灯・モータ励磁OFF
	DMset(0);                     // DCモータ停止
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { tc = 0; }
}

///////////////////////////////////////////////////////////////////////////////
// タクトスイッチのクリック／ダブルクリック／長押し判定
//   戻り値: KEY_NONE / KEY_BUSY / KEY_SINGLE / KEY_DOUBLE / KEY_LONG
//   KEY_SINGLE，KEY_DOUBLE，KEY_LONG は1イベントにつき1回だけ返る
///////////////////////////////////////////////////////////////////////////////
int KeySense(void) {
	boolean sw = tsw;  // volatile変数は1回だけ読む
	int t;

	switch (kst) {
	case 0:  // 押下待ち
		if (sw == LOW) {
			tswFlag = true;
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { kTime = T_DBLCLICK; }
			kst = 1;
			return KEY_BUSY;
		}
		return KEY_NONE;

	case 1:  // ダブルクリック判定中（押下から500ms）
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { t = kTime; }
		if (t > 0) {
			if (sw == HIGH) tswFlag = false;          // 1回目の押下が離された
			if ((sw == LOW) && (tswFlag == false)) {  // 2回目の押下
				kst = 3;
				beep(4000);
				return KEY_DOUBLE;
			}
			return KEY_BUSY;
		}
		if (tswFlag == false) {  // 一度離されている → シングル確定
			kst = 3;
			beep(1000);
			return KEY_SINGLE;
		}
		// 500ms押されたまま → 長押し判定へ
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { kTime = T_LONGPRESS - T_DBLCLICK; }
		kst = 2;
		return KEY_BUSY;

	case 2:  // 長押し判定中（押下から500～1000ms）
		if (sw == HIGH) {  // 1000ms未満で離された → シングル確定
			kst = 0;
			beep(1000);
			return KEY_SINGLE;
		}
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { t = kTime; }
		if (t == 0) {  // 1000ms押し続けた → 長押し確定
			kst = 3;
			beep(4500);
			return KEY_LONG;
		}
		return KEY_BUSY;

	case 3:  // 入力確定後，タクトスイッチのoffを待つ
		if (sw == HIGH) kst = 0;
		return KEY_NONE;

	default:
		kst = 0;
		return KEY_NONE;
	}
}

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
void loop(void) {
	int kCode = KeySense();
	word t;

	///// 長押し：初期状態に戻る /////
	if (kCode == KEY_LONG) {
		devNo = 1;
		value = 0;
		disp(num[devNo], num[value]);
		allStop();
	}

	///// ダブルクリック：デバイス切替 /////
	if (kCode == KEY_DOUBLE) {
		devNo++;
		if (devNo > 3) devNo = 1;
		value = 0;
		disp(num[devNo], num[value]);
		allStop();
	}

	switch (devNo) {
	case 1:  // FullColor LED
		if (kCode == KEY_SINGLE) {
			value++;
			if (value >= COUNT_OF(ledPara)) value = 0;
			disp(num[devNo], num[value]);
			lm.led(ledPara[value]).flush();
		}
		break;

	case 2:  // Stepping motor
		if (kCode == KEY_SINGLE) {
			value++;
			if (value >= COUNT_OF(smPara)) value = 0;
			disp(num[devNo], num[value]);
		}
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { t = tc; }
		if (smPara[value] == -1) {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { tc = 0; }
			lm.spm(STOP).flush();
		} else if (smPara[value] > 0) {
			if (t >= (word)smPara[value]) {
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { tc = 0; }
				lm.spm(CW).flush();
			}
		} else {
			if (t >= (word)(-smPara[value])) {
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { tc = 0; }
				lm.spm(CCW).flush();
			}
		}
		break;

	case 3:  // DC motor
		if (kCode == KEY_SINGLE) {
			value++;
			if (value >= COUNT_OF(dmPara)) value = 0;
			disp(num[devNo], num[value]);
			DMset(dmPara[value]);
		}
		break;

	default:
		break;
	}
}
