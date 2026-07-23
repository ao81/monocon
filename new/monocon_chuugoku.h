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

//================ ピン定義 ================
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

#define BZ_PIN    5
#define LED_G_PIN 2
#define LED_B_PIN 3
#define LED_R_PIN 4

#define SPM1_PIN 28
#define SPM2_PIN 26
#define SPM3_PIN 24
#define SPM4_PIN 22

// PWM対応ピン
#define DCM1_PIN 44
#define DCM2_PIN 46

#define PH_PIN 36

// ピン6/7/8 = PORTH(PH3/PH4/PH5)
#define SCK_BIT (1 << PH3)
#define SDI_BIT (1 << PH4)
#define LAT_BIT (1 << PH5)

//================ 低レベルI/O ================
// shiftOut の高速版（PORTH直叩き, MSBファースト）
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

// analogRead の高速版
inline int ar(uint8_t pin) {
	if (pin >= A0) pin -= A0;
	ADMUX = (1 << REFS0) | (pin & 0x07);
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 1) << MUX5);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
}

// digitalRead の高速版
inline int dr(uint8_t pin) {
	return (*portInputRegister(digitalPinToPort(pin)) & digitalPinToBitMask(pin)) ? HIGH : LOW;
}

// 7セグ用フォント (0-9, A-F)
const uint8_t seg[16] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66,
	0x6d, 0x7d, 0x27, 0x7f, 0x6f,
	0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71,
};

//================ 汎用 ================
// v を [lo, hi] に収める
inline long clampv(long v, long lo, long hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}

// cur を target へ最大 step だけ近づける（ソフトスタート用）
inline long toward(long cur, long target, long step) {
	if (cur < target) return (cur + step > target) ? target : cur + step;
	if (cur > target) return (cur - step < target) ? target : cur - step;
	return cur;
}

// 中央±width 内なら center を返す不感帯処理
inline long deadband(long v, long center, long width) {
	return (v > center - width && v < center + width) ? center : v;
}

//================ 入力 ================
// 使い方:
//   de e = in.d(d1);
//   if (e.ltoh) { ... }        // 押した瞬間 (LOW→HIGH)
//   if (e.htol) { ... }        // 離した瞬間 (HIGH→LOW)
//   int lv = e.level;          // トグルSW等は level だけ見る
//   int v  = in.an(a1);        // アナログ全般
//   Joy j  = in.joy(a1, a2);   // j.dir() で方向 (0=上, 時計回り)
//   long c = in.enc(d2, d3);   // エンコーダ累積カウント
// 注意:
//   in.d() / in.enc() は loop で毎回呼ぶ（delay で止めない）
//   d() は同じピンを1ループ1回だけ呼ぶ

struct de {
	uint8_t level;   // 現在レベル (HIGH/LOW)
	bool    ltoh;    // LOW→HIGH の瞬間だけ true
	bool    htol;    // HIGH→LOW の瞬間だけ true
	operator int() const { return level; }
};

struct Joy {
	int x, y;
	// div 分割で 0..div-1 を返す。上=0, 時計回り。中央付近(半径≒141)は -1。
	int dir(int div = 4) const {
		long dx = x - 511, dy = y - 511;
		if (dx * dx + dy * dy < 20000) return -1;
		return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / div) / (2 * PI / div)) % div;
	}
};

class In {
	// デジタルチャンネル（ピンごとに独立した状態）
	struct Dch {
		uint8_t           pin;
		volatile uint8_t* reg;     // 入力レジスタ（初回だけ解決）
		uint8_t           mask;
		uint8_t           stable;  // 確定済みレベル
		unsigned long     t;       // 最後に確定した時刻(ms)
		bool              init;
	};
	static const uint8_t NCH = 8;
	Dch ch[NCH] = {};

	Dch* slot(uint8_t pin) {
		Dch* freeCh = nullptr;
		for (uint8_t i = 0; i < NCH; i++) {
			if (ch[i].init && ch[i].pin == pin) return &ch[i];
			if (!ch[i].init && !freeCh) freeCh = &ch[i];
		}
		if (!freeCh) return nullptr;
		freeCh->pin = pin;
		freeCh->reg = portInputRegister(digitalPinToPort(pin));
		freeCh->mask = digitalPinToBitMask(pin);
		freeCh->stable = (*freeCh->reg & freeCh->mask) ? HIGH : LOW;
		freeCh->t = 0;
		freeCh->init = true;
		return freeCh;
	}

	// エンコーダ（1系統）
	uint8_t           est = 0;
	volatile uint8_t* erA = nullptr; uint8_t emA = 0;
	volatile uint8_t* erB = nullptr; uint8_t emB = 0;

public:
	// デジタル入力＋エッジ検出。lock = チャタリング無視時間(ms)。
	de d(uint8_t pin, uint16_t lock = 10) {
		Dch* c = slot(pin);
		if (!c) return { LOW, false, false };
		uint8_t raw = (*c->reg & c->mask) ? HIGH : LOW;
		de r = { c->stable, false, false };
		if (raw == c->stable) return r;
		unsigned long now = millis();
		if (now - c->t < lock) return r;
		r.ltoh = (c->stable == LOW && raw == HIGH);
		r.htol = (c->stable == HIGH && raw == LOW);
		r.level = raw;
		c->stable = raw;
		c->t = now;
		return r;
	}

	// アナログ入力
	int an(uint8_t pin) {
		return ar(pin);
	}

	// ジョイスティック (X/Y 一括読み)
	Joy joy(uint8_t px, uint8_t py) {
		Joy j;
		j.x = ar(px);
		j.y = ar(py);
		return j;
	}

	long c = 0;   // エンコーダ累積カウント

	// 1ステップ分をデコードして -1/0/+1 を返す（c は触らない）
	int encDelta(uint8_t pa, uint8_t pb, bool dir) {
		if (!erA) {
			erA = portInputRegister(digitalPinToPort(pa)); emA = digitalPinToBitMask(pa);
			erB = portInputRegister(digitalPinToPort(pb)); emB = digitalPinToBitMask(pb);
		}
		uint8_t a = (*erA & emA) ? 1 : 0;
		uint8_t b = (*erB & emB) ? 1 : 0;
		// 判定点 AB=00 の7状態テーブル
		static const uint8_t table[7][4] = {
			{ 0x00, 0x02, 0x04, 0x00 },
			{ 0x00, 0x00, 0x00, 0x00 },
			{ 0x13, 0x02, 0x00, 0x00 },
			{ 0x03, 0x03, 0x03, 0x00 },
			{ 0x26, 0x00, 0x04, 0x00 },
			{ 0x00, 0x00, 0x00, 0x00 },
			{ 0x06, 0x06, 0x06, 0x00 },
		};
		est = table[est & 0x0f][(a << 1) | b];
		uint8_t dd = est & 0x30;
		if (dd == 0x10)      return dir ? 1 : -1;
		else if (dd == 0x20) return dir ? -1 : 1;
		return 0;
	}

	// 累積カウントを返す。dir=false で回転方向を反転。
	long enc(uint8_t pa, uint8_t pb, bool dir = true) {
		c += encDelta(pa, pb, dir);
		return c;
	}

	// [lo, hi] で止まる版（端で空回りしない）
	long encClamp(uint8_t pa, uint8_t pb, long lo, long hi, bool dir = true) {
		c += encDelta(pa, pb, dir);
		c = clampv(c, lo, hi);
		return c;
	}

	// [lo, hi] を循環する版。hi の次は lo に戻る。
	long encLoop(uint8_t pa, uint8_t pb, long lo, long hi, bool dir = true) {
		c += encDelta(pa, pb, dir);
		long span = hi - lo + 1;
		if (span < 1) span = 1;
		c = lo + ((c - lo) % span + span) % span;
		return c;
	}
};
In in;

// 測距モジュール: アナログ値 → 距離cm (4〜50cm にクランプ)
int sok(int pin) {
	return (int)clampv(map(ar(pin), 10, 450, 50, 4), 4, 50);
}

//================ ノンブロッキング・タイミング ================
// 一定間隔 ms ごとに true。
//   iv t;  if (t(500)) { 0.5秒ごとの処理 }
class iv {
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
//   ti t;  t.start(1000);  ... if (t.done()) { 1秒後の処理 }
class ti {
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
	// 残り時間(ms)。停止中・経過後は 0。カウントダウン表示用。
	unsigned long remain() {
		if (!run) return 0;
		long r = (long)(lim - millis());
		return r > 0 ? (unsigned long)r : 0;
	}
};

//================ 7セグ (3桁) ================
// 生パターンを直接送る。前回と同じなら送信しない。
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

// 整数をそのまま表示 (-99〜999)。先頭ゼロ消灯・負数は符号付き。
void dispn(int n) {
	bool neg = n < 0;
	if (neg) n = -n;
	if (neg && n > 99) n = 99;
	if (n > 999) n = 999;
	uint8_t bh = (n >= 100) ? seg[(n / 100) % 10] : 0x00;
	uint8_t bt = (n >= 10) ? seg[(n / 10) % 10] : 0x00;
	uint8_t bo = seg[n % 10];
	if (neg) {
		if (n < 10) bt = 0x40;
		else        bh = 0x40;
	}
	disp(bh, bt, bo);
}

// 小数を自動桁数で表示。値の大きさで小数点位置が変わる。
//   dispDec(0.5f)  → "0.50"    dispDec(23.4f) → "23.4"
//   dispDec(123.0f)→ "123"     dispDec(-1.5f) → "-1.5"
// 正:  <10 で2桁, <100 で1桁, それ以上は整数(最大999)
// 負:  <10 で1桁, それ以上は整数(最小-99)
void dispn(double f) {
	bool neg = f < 0;
	double a = neg ? -f : f;

	if (neg) {
		if (a < 9.95) {
			int v = (int)(a * 10 + 0.5);
			if (v > 99) v = 99;
			disp(0x40, seg[(v / 10) % 10] | 0x80, seg[v % 10]);
		} else {
			int v = (int)(a + 0.5);
			if (v > 99) v = 99;
			disp(0x40, seg[(v / 10) % 10], seg[v % 10]);
		}
	} else {
		if (a < 9.995) {
			int v = (int)(a * 100 + 0.5);
			if (v > 999) v = 999;
			disp(seg[(v / 100) % 10] | 0x80, seg[(v / 10) % 10], seg[v % 10]);
		} else if (a < 99.95) {
			int v = (int)(a * 10 + 0.5);
			if (v > 999) v = 999;
			disp(seg[(v / 100) % 10], seg[(v / 10) % 10] | 0x80, seg[v % 10]);
		} else {
			int v = (int)(a + 0.5);
			if (v > 999) v = 999;
			disp(seg[(v / 100) % 10], seg[(v / 10) % 10], seg[v % 10]);
		}
	}
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

// 音階定数 (Hz)
#define nc4 262
#define nd4 294
#define ne4 330
#define nf4 349
#define ng4 392
#define na4 440
#define nb4 494
#define nc5 523
#define nd5 587
#define ne5 659
#define nf5 698
#define ng5 784
#define na5 880
#define nc6 1047
#define nr  0

// 非ブロッキング・メロディ再生。
//   const int n[] = { NOTE_C4, NOTE_E4, NOTE_G4 };
//   const int d[] = { 200, 200, 400 };   // 各音の長さ(ms)
//   mel.play(n, d, 3);   ... loop 内で mel.update();
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
		// tone に長さを渡し自動停止させる → 音間に実際の無音20msが入る
		if (ns[idx] > 0) tone(BZ_PIN, ns[idx], ds[idx]);
		else noTone(BZ_PIN);
		next = now + ds[idx];
		idx++;
	}
};
Melody mel;

//================ フルカラーLED ================
#define G 0b100
#define B 0b010
#define R 0b001
#define GB 0b110
#define GR 0b101
#define BR 0b011
#define GBR 0b111

class Led {
public:
	void operator()(uint8_t m) {
		digitalWrite(LED_R_PIN, (m & 1) ? HIGH : LOW);
		digitalWrite(LED_B_PIN, (m & 2) ? HIGH : LOW);
		digitalWrite(LED_G_PIN, (m & 4) ? HIGH : LOW);
	}
};
Led led;

//================ ステッピングモータ (28BYJ-48, 2相励磁) ================
class Spm {
	int8_t ix = 0;
	long   ps = 0;
	unsigned long stepPre = 0;

public:
	long rem = 0;   // 残ステップ数

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
		set((ix + 3) & 3);
		ps++;
	}

	void ccw() {
		set((ix + 1) & 3);
		ps--;
	}

	// 全相OFF（脱励磁）
	void off() {
		digitalWrite(SPM1_PIN, LOW);
		digitalWrite(SPM2_PIN, LOW);
		digitalWrite(SPM3_PIN, LOW);
		digitalWrite(SPM4_PIN, LOW);
	}

	// 相対移動をキューに積む（正=CW, 負=CCW）
	void mv(long n) {
		rem += n;
	}

	// 残移動を打ち切る（励磁は保持）
	void stop() {
		rem = 0;
	}

	// 1ステップ進める。速度制御なしで最速駆動したいとき用。
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

	// 絶対位置 t へ移動（直線軸）
	void to(long t) {
		rem = t - ps;
	}

	// 円環軸（1周 period ステップ）で target へ最短方向に回す（rem に加算）
	void turn(long target, long period) {
		long cur = ((ps % period) + period) % period;
		long tgt = ((target % period) + period) % period;
		long d = ((tgt - cur) % period + period) % period;
		if (d > period / 2) d -= period;
		mv(d);
	}

	// 円環軸で target へ最短方向。rem を代入し直す版（毎ループ呼んでよい）
	void seek(long target, long period) {
		long cur = ((ps % period) + period) % period;
		long tgt = ((target % period) + period) % period;
		long d = ((tgt - cur) % period + period) % period;
		if (d > period / 2) d -= period;
		rem = d;
	}

	// 速度制御つき非ブロッキング駆動。ms ミリ秒ごとに1ステップ。
	//   spm.mv(2048);  ... loop 内で spm.step(2);   // 2ms/step
	void step(unsigned long ms) {
		if (rem == 0) return;
		unsigned long now = millis();
		if (now - stepPre < ms) return;
		stepPre = now;
		run();
	}
};
Spm sm;

// 円環軸での最短移動量 (-period/2 〜 period/2)
int getmove(int from, int to, int period = 2048) {
	int d = ((to - from) % period + period) % period;
	if (d > period / 2) d -= period;
	return d;
}

//================ DCモータ (DRV8835) ================
class Dcm {
	int cs = 0;   // ramp 用の現在速度

public:
	void cw(int spd) {
		digitalWrite(DCM1_PIN, LOW);
		analogWrite(DCM2_PIN, spd);
	}

	void ccw(int spd) {
		analogWrite(DCM1_PIN, spd);
		digitalWrite(DCM2_PIN, LOW);
	}

	void br() {
		digitalWrite(DCM1_PIN, HIGH);
		digitalWrite(DCM2_PIN, HIGH);
	}

	void fr() {
		digitalWrite(DCM1_PIN, LOW);
		digitalWrite(DCM2_PIN, LOW);
	}

	// 符号付き速度。spd>0:正転 spd<0:逆転 spd=0:ブレーキ。±255 に自動クランプ。
	void drive(int spd) {
		if (spd > 0) {
			cw(spd > 255 ? 255 : spd);
		} else if (spd < 0) {
			ccw(-spd > 255 ? 255 : -spd);
		} else {
			br();
		}
	}

	// ソフトスタート付き drive。target へ毎回 step ずつ近づける。
	// loop 内で iv 等と組み合わせて周期的に呼ぶ。
	//   if (t(20)) dm.ramp(200, 10);   // 20msごとに10ずつ加速
	void ramp(int target, int step) {
		cs = (int)toward(cs, target, step);
		drive(cs);
	}

	// ramp の現在速度
	int speed() { return cs; }
};
Dcm dm;

//================ シーケンス制御 ================
// loop の中にステップを順に並べる。記述順に 0,1,2... と番号が振られ、
// 現在ステップの if だけが実行される。
//
//   Seq sq;
//   void loop() {
//     sq.top();                          // loop 先頭で必ず1回
//     if (sq.on()) {                     // ステップ0
//       if (sq.in()) led(C_RED);         //   入った瞬間だけ
//       if (sq.after(1000)) sq.next();   //   1秒たったら次へ
//     }
//     if (sq.on()) {                     // ステップ1
//       if (in.d(d1).ltoh) sq.next();    //   条件で次へ
//     }
//     if (sq.on()) {                     // ステップ2
//       if (sq.after(2000)) sq.to(0);    //   先頭へ戻る
//     }
//   }
class Seq {
	int  cur = 0;
	int  pos = 0;
	int  count = 0;
	unsigned long t0 = 0;
	bool moved = false;
	bool exited = false;
public:
	bool fresh = true;

	void top() {
		if (pos > count) count = pos;
		pos = 0;
		moved = false;
		exited = false;
	}
	bool on() {
		if (moved) { pos++; return false; }
		return (pos++ == cur);
	}
	bool operator()() { return on(); }

	void next() {
		cur++;
		if (count > 0 && cur >= count) cur = 0;
		t0 = millis(); fresh = true; moved = true;
	}
	void prev() {
		cur--;
		if (cur < 0) cur = (count > 0) ? count - 1 : 0;
		t0 = millis(); fresh = true; moved = true;
	}

	void to(int s) { cur = s; t0 = millis(); fresh = true; moved = true; }
	bool is(int s) { return cur == s; }
	int  now() { return cur; }
	int  steps() { return count; }   // 計測済みのステップ総数
	bool in() {
		if (fresh) { fresh = false; return true; }
		return false;
	}
	bool out() {
		if (moved && !exited) { exited = true; return true; }
		return false;
	}
	unsigned long elapsed() { return millis() - t0; }
	bool after(unsigned long ms) { return millis() - t0 >= ms; }
};

//================ Timer3 割り込み ================
// 使うときは #include より前に #define timer3 を書き、void isr() を実装する。
// 100μs 周期で isr() が呼ばれる。
#ifdef timer3
void isr();
ISR(TIMER3_COMPA_vect) {
	isr();
}
#endif

//================ 初期化 ================
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

	sm.off();
	dm.fr();
	dispOff();

	randomSeed(millis());

	// ADC 分解能を維持したまま変換を高速化 (分周比16)
	ADCSRA = (ADCSRA & ~0x07) | (1 << ADPS2);

#ifdef timer3
	cli();
	TCCR3A = TCCR3B = TCNT3 = 0;
	OCR3A = F_CPU / 64 / 10000 - 1;   // 100μs
	TCCR3B |= (1 << WGM32);
	TCCR3B |= (1 << CS31) | (1 << CS30);
	TIMSK3 |= (1 << OCIE3A);
	sei();
#endif
}
