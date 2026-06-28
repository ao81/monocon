/**********************************************/
// 高校生ものづくりコンテスト2026全国大会
//
// 地区名: 中国地区
// 学校名: 岡山県立岡山工業高等学校
// 氏名: 青山 晃大
// 作成年月日: 2026/〇〇/〇〇
/**********************************************/

// これ以下のコメント文は後に全て削除すること

#pragma once

#define a1 A0
#define a2 A1
#define a3 A2
#define a4 A3
#define d1 10
#define d2 11
#define d3 12
#define d4 13

#define SCK_PIN 6
#define SDI_PIN 7
#define LAT_PIN 8

#define BZ_PIN 5
#define LED_G_PIN 2
#define LED_B_PIN 3
#define LED_R_PIN 4

#define SPM1_PIN 28
#define SPM2_PIN 26
#define SPM3_PIN 24
#define SPM4_PIN 22

///////////////////////////////
// PWM出力に対応したピンに変更 //
///////////////////////////////
#define DCM1_PIN 44
#define DCM2_PIN 46

#define PH_PIN 36

// ピン6/7/8 = PORTH(PH3/PH4/PH5)
#define SCK_BIT (1 << PH3)
#define SDI_BIT (1 << PH4)
#define LAT_BIT (1 << PH5)

inline void fsout(uint8_t v) {
	for (uint8_t i = 0; i < 8; i++) {
		if (v & 0x80) {
			PORTH |= SDI_BIT;
		} else {
			PORTH &= ~SDI_BIT;
		}
		PORTH |= SCK_BIT;
		PORTH &= ~SCK_BIT;
		v <<= 1;
	}
}

inline int ar(uint8_t pin) {
	if (pin >= A0) pin -= A0;
	ADMUX = (1 << REFS0) | (pin & 0x07);
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 1) << MUX5);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

inline int dr(uint8_t pin) {
	return (*portInputRegister(digitalPinToPort(pin)) & digitalPinToBitMask(pin)) ? HIGH : LOW;
}

const uint8_t seg[16] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66,
	0x6d, 0x7d, 0x27, 0x7f, 0x6f,
	0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71,
};

//================ 入力 ================
class In {
public:
	/* タクトスイッチ */
	int sw;
	void fsw(int pin) {
		sw = dr(pin);
	}

	/* トグルスイッチ */
	int ts;
	void fts(int pin) {
		ts = dr(pin);
	}

	/* フォトインタラプタ */
	int ph;
	void fph(int pin) {
		ph = dr(pin);
	}

	/* 測距モジュール */
	int rm;
	void frm(int pin) {
		rm = ar(pin);
	}

	/* ロータリーエンコーダ */
	int a = 0, b = 0;
	int pa = 0, pb = 0;
	int c = 0;
	uint8_t st = 0;
	void fen(int pinA, int pinB, bool d = true) {
		a = dr(pinA);
		b = dr(pinB);
		static const uint8_t table[7][4] = {
			{ 0x00, 0x02, 0x04, 0x00 },
			{ 0x00, 0x00, 0x00, 0x00 },
			{ 0x13, 0x02, 0x00, 0x00 },
			{ 0x03, 0x03, 0x03, 0x00 },
			{ 0x26, 0x00, 0x04, 0x00 },
			{ 0x00, 0x00, 0x00, 0x00 },
			{ 0x06, 0x06, 0x06, 0x00 },
		};
		uint8_t pin = (a << 1) | b;
		st = table[st & 0x0f][pin];
		uint8_t dir = st & 0x30;
		if (dir == 0x10) {
			if (d) {
				c++;
			} else {
				c--;
			}
		} else if (dir == 0x20) {
			if (d) {
				c--;
			} else {
				c++;
			}
		}
		pa = a;
		pb = b;
	}

	/* フォトリフレクタ */
	int p = 0;
	void fpr(int pin) {
		p = ar(pin);
	}

	/* ジョイスティック */
	int x = 0, y = 0;
	void js(int pinX, int pinY) {
		x = ar(pinX);
		y = ar(pinY);
	}
};
In in;

int& sw = in.sw;
int& ts = in.ts;
int& ph = in.ph;
int& rm = in.rm;
int& cnt = in.c;
int& p = in.p;
int& x = in.x;
int& y = in.y;

//================ ジョイスティック方向判定 ================
// 現在の x,y を div 方向へ分割して 0..div-1 を返す。
// 上が0で時計回りに増加。中央付近（半径≒141以内）は -1（入力なし）。
//   in.js(a1, a2);  int d = jsdir(4);   // 0上 1右 2下 3左
int jsdir(int div = 4) {
	long dx = x - 511, dy = y - 511;
	if (dx * dx + dy * dy < 20000) return -1;
	return (int)((atan2(dx, dy) + 2 * PI + PI / div) / (2 * PI / div)) % div;
}

//================ エッジ検出（押した/離した瞬間） ================
// 立ち上がり(LOW→HIGH)・立ち下がり(HIGH→LOW)を検出する。
// スイッチの極性に依存せず、必要な方(rise/fall)を使えばよい。
//   Edge btn;  ... btn.f(d1);  if (btn.rise) { 押した瞬間の処理 }
class Edge {
	int prev = -1;
public:
	int  val  = 0;      // 現在値 (HIGH/LOW)
	bool rise = false;  // LOW→HIGH になった瞬間だけ true
	bool fall = false;  // HIGH→LOW になった瞬間だけ true
	void f(int pin) {
		val = dr(pin);
		if (prev < 0) prev = val;
		rise = (!prev &&  val);
		fall = ( prev && !val);
		prev = val;
	}
};

//================ ノンブロッキング・タイミング ================
// 一定間隔 ms ごとに true を返す。delay() を使わず周期処理に使う。
//   Every t;  if (t(500)) { 0.5秒ごとの処理 }
class Every {
	unsigned long pre = 0;
public:
	bool operator()(unsigned long ms) {
		unsigned long now = millis();
		if (now - pre >= ms) {
			pre = now;
			return true;
		}
		return false;
	}
	void reset() { pre = millis(); }
};

// ワンショットタイマ。start(ms) 後、経過した瞬間に done() が一度だけ true。
//   Timer t;  t.start(1000);  ... if (t.done()) { 1秒後の処理 }
class Timer {
	unsigned long lim = 0;
	bool run = false;
public:
	void start(unsigned long ms) { lim = millis() + ms; run = true; }
	void stop() { run = false; }
	bool active() { return run; }
	bool done() {
		if (run && (long)(millis() - lim) >= 0) {
			run = false;
			return true;
		}
		return false;
	}
};

//================ 7セグ（3桁） ================
void disp(char a, char b, char c) {
	static int pa = -1, pb = -1, pc = -1;
	if (pa != (uint8_t)a || pb != (uint8_t)b || pc != (uint8_t)c) {
		pa = (uint8_t)a;
		pb = (uint8_t)b;
		pc = (uint8_t)c;
		PORTH &= ~LAT_BIT;
		fsout(a);
		fsout(b);
		fsout(c);
		PORTH |= LAT_BIT;
	}
}

//================ 7セグ 数値表示ヘルパ ================
// 整数表示 (-99〜999)。先頭ゼロは消灯し、負数は符号付きで右詰め表示。
void dispNum(int n) {
	bool neg = n < 0;
	if (neg) n = -n;
	if (n > 999) n = 999;
	uint8_t bh = (n >= 100) ? seg[(n / 100) % 10] : 0x00;
	uint8_t bt = (n >= 10)  ? seg[(n / 10) % 10]  : 0x00;
	uint8_t bo = seg[n % 10];
	if (neg) {
		if (n < 10)       bt = 0x40;   // 1桁: 中央に "-"
		else if (n < 100) bh = 0x40;   // 2桁: 左に "-"
		// 3桁の負数は桁あふれのため符号は省略
	}
	disp(bh, bt, bo);
}

// 小数点付き表示。point=小数部の桁数 (1 or 2)。
//   dispDec(234, 1) → "23.4"   dispDec(50, 2) → "0.50"
void dispDec(int v, int point = 1) {
	if (v < 0) v = 0;
	if (v > 999) v = 999;
	uint8_t b0 = seg[(v / 100) % 10];
	uint8_t b1 = seg[(v / 10) % 10];
	uint8_t b2 = seg[v % 10];
	if (point == 2) {
		b0 |= 0x80;
	} else {
		b1 |= 0x80;
		if (v < 100) b0 = 0x00;        // 先頭ゼロ消灯
	}
	disp(b0, b1, b2);
}

// 全消灯
void dispOff() {
	disp(0, 0, 0);
}

//================ ブザー ================
void bz(int f) {
	tone(BZ_PIN, f);
}
void bz(int f, unsigned long t) {
	tone(BZ_PIN, f, t);
}
void bzoff() {
	noTone(BZ_PIN);
}

// 音階定数（Hz）
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_C6 1047
#define NOTE_REST 0

// 正解音（上昇）／不正解音（下降）。短時間ブロッキングする一発フィードバック用。
inline void bzWin() {
	tone(BZ_PIN, NOTE_C5, 80); delay(90);
	tone(BZ_PIN, NOTE_E5, 80); delay(90);
	tone(BZ_PIN, NOTE_G5, 140); delay(150);
	noTone(BZ_PIN);
}
inline void bzNg() {
	tone(BZ_PIN, NOTE_G4, 120); delay(130);
	tone(BZ_PIN, NOTE_C4, 220); delay(230);
	noTone(BZ_PIN);
}

// 非ブロッキング・メロディ再生。loop() を止めずに鳴らす。
//   const int n[] = { NOTE_C4, NOTE_E4, NOTE_G4 };
//   const int d[] = { 200, 200, 400 };           // 各音の長さ(ms)
//   melody.play(n, d, 3);   ... loop 内で melody.update();
class Melody {
	const int* ns = nullptr;
	const int* ds = nullptr;
	int len = 0, idx = 0;
	unsigned long next = 0;
	bool run = false;
public:
	void play(const int* notes, const int* durs, int n) {
		ns = notes; ds = durs; len = n; idx = 0; run = true; next = 0;
	}
	void stop() { run = false; noTone(BZ_PIN); }
	bool playing() { return run; }
	void update() {
		if (!run) return;
		unsigned long now = millis();
		if ((long)(now - next) < 0) return;
		if (idx >= len) { run = false; noTone(BZ_PIN); return; }
		if (ns[idx] > 0) tone(BZ_PIN, ns[idx]);
		else noTone(BZ_PIN);
		next = now + ds[idx] + 20;   // 音の間に小さな余白
		idx++;
	}
};
Melody melody;

//================ フルカラーLED ================
class Led {
public:
	void operator()(uint8_t m) {
		digitalWrite(LED_R_PIN, (m & 1) ? HIGH : LOW);
		digitalWrite(LED_G_PIN, (m & 2) ? HIGH : LOW);
		digitalWrite(LED_B_PIN, (m & 4) ? HIGH : LOW);
	}
	void off() {
		(*this)(0);
	}
};
Led led;

//================ ステッピングモータ (28BYJ-48, 2相励磁) ================
class Spm {
	int8_t ix = 0;
	long   ps = 0;
	int    rem = 0;
	unsigned long stepPre = 0;
public:
	void set(uint8_t s) {
		static const uint8_t tbl[4] = { 0b1001, 0b1100, 0b0110, 0b0011 };  // SPM4..1
		uint8_t b = tbl[s & 3];
		digitalWrite(SPM1_PIN, (b & 1) ? HIGH : LOW);
		digitalWrite(SPM2_PIN, (b & 2) ? HIGH : LOW);
		digitalWrite(SPM3_PIN, (b & 4) ? HIGH : LOW);
		digitalWrite(SPM4_PIN, (b & 8) ? HIGH : LOW);
		ix = s & 3;
	}
	void cw() {
		set((ix + 1) & 3);
		ps++;
	}
	void ccw() {
		set((ix + 3) & 3);
		ps--;
	}
	void off() {
		digitalWrite(SPM1_PIN, LOW);
		digitalWrite(SPM2_PIN, LOW);
		digitalWrite(SPM3_PIN, LOW);
		digitalWrite(SPM4_PIN, LOW);
	}
	void mv(int n) {
		rem += n;
	}
	void run() {
		if (rem > 0) {
			cw();
			rem--;
		} else if (rem < 0) {
			ccw();
			rem++;
		}
	}
	bool moving() {
		return rem != 0;
	}
	long pos() {
		return ps;
	}
	void zero() {
		ps = 0;
		rem = 0;
	}

	// 絶対位置 t へ移動（直線軸）。最終到達位置が t になるよう残ステップを設定。
	void to(long t) {
		rem = (int)(t - ps);
	}
	// 円環軸（1周 period ステップ）で目標 target へ最短方向に回す。
	void turn(long target, long period) {
		long cur = ((ps % period) + period) % period;
		long tgt = ((target % period) + period) % period;
		long d = ((tgt - cur) % period + period) % period;
		if (d > period / 2) d -= period;
		mv((int)d);
	}
	// 速度制御つき非ブロッキング駆動。ms ミリ秒ごとに1ステップ進める。
	//   spm.mv(2048);  ... loop 内で spm.step(2);   // 2ms/step
	void step(unsigned long ms) {
		if (rem == 0) return;
		unsigned long now = millis();
		if (now - stepPre < ms) return;
		stepPre = now;
		run();
	}
};
Spm spm;

// 円環軸での最短移動量を返す（snippets 互換, 既定 period=120）。
//   戻り値: -period/2 〜 period/2
int getmove(int from, int to, int period = 120) {
	int d = ((to - from) % period + period) % period;
	if (d > period / 2) d -= period;
	return d;
}

//================ DCモータ (DRV8835) ================
#define CW  0xff
#define CCW 0xfe
#define BR  0xfd
#define FR  0xfc

class Dcm {
public:
	void cw(int spd) {
		analogWrite(DCM1_PIN, spd);
		digitalWrite(DCM2_PIN, LOW);
	}
	void ccw(int spd) {
		digitalWrite(DCM1_PIN, LOW);
		analogWrite(DCM2_PIN, spd);
	}
	void br() {
		digitalWrite(DCM1_PIN, HIGH);
		digitalWrite(DCM2_PIN, HIGH);
	}
	void fr() {
		digitalWrite(DCM1_PIN, LOW);
		digitalWrite(DCM2_PIN, LOW);
	}
	// void operator()(uint8_t m) {
	// 	switch (m) {
	// 	case CW:
	// 		cw();
	// 		break;
	// 	case CCW:
	// 		ccw();
	// 		break;
	// 	case BR:
	// 		br();
	// 		break;
	// 	default:
	// 		fr();
	// 		break;
	// 	}
	// }
};
Dcm dcm;

//================ Timer3 割り込み ================
void isr();
ISR(TIMER3_COMPA_vect) {
	isr();
}

void begin(void) {
	pinMode(a1, INPUT);
	pinMode(a2, INPUT);
	pinMode(a3, INPUT);
	pinMode(a4, INPUT);
	pinMode(d1, INPUT);
	pinMode(d2, INPUT);
	pinMode(d3, INPUT);
	pinMode(d4, INPUT);

	pinMode(LAT_PIN, OUTPUT);
	pinMode(SCK_PIN, OUTPUT);
	pinMode(SDI_PIN, OUTPUT);
	digitalWrite(LAT_PIN, HIGH);
	digitalWrite(SCK_PIN, LOW);
	digitalWrite(SDI_PIN, LOW);

	pinMode(BZ_PIN, OUTPUT);
	pinMode(LED_R_PIN, OUTPUT);
	pinMode(LED_G_PIN, OUTPUT);
	pinMode(LED_B_PIN, OUTPUT);
	pinMode(SPM1_PIN, OUTPUT);
	pinMode(SPM2_PIN, OUTPUT);
	pinMode(SPM3_PIN, OUTPUT);
	pinMode(SPM4_PIN, OUTPUT);
	pinMode(DCM1_PIN, OUTPUT);
	pinMode(DCM2_PIN, OUTPUT);
	pinMode(PH_PIN, INPUT);

	led.off();
	spm.off();
	dcm.fr();

	ADCSRA = (ADCSRA & ~0x07) | (1 << ADPS2);

	cli();
	TCCR3A = TCCR3B = TCNT3 = 0;
	OCR3A = F_CPU / 64 / 10000 - 1; // 100μs
	TCCR3B |= (1 << WGM32);
	TCCR3B |= (1 << CS31) | (1 << CS30);
	TIMSK3 |= (1 << OCIE3A);
	sei();
}
