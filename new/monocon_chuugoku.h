/**********************************************/
// 高校生ものづくりコンテスト2026全国大会
//
// 地区名: 中国地区
// 学校名: 岡山県立岡山工業高等学校
// 氏名: 青山 晃大
// 作成年月日: 2026/07/11
/**********************************************/

// 以下のコメント文は全て削除すること

#pragma once

#include <util/atomic.h>

constexpr uint8_t a1 = A0;
constexpr uint8_t a2 = A1;
constexpr uint8_t a3 = A2;
constexpr uint8_t a4 = A3;
constexpr uint8_t d1 = 10;
constexpr uint8_t d2 = 11;
constexpr uint8_t d3 = 12;
constexpr uint8_t d4 = 13;

constexpr uint8_t SCK_PIN = 6;
constexpr uint8_t SDI_PIN = 7;
constexpr uint8_t LAT_PIN = 8;

constexpr uint8_t BZ_PIN = 5;
constexpr uint8_t LED_G_PIN = 2;
constexpr uint8_t LED_B_PIN = 3;
constexpr uint8_t LED_R_PIN = 4;

constexpr uint8_t SPM1_PIN = 28;
constexpr uint8_t SPM2_PIN = 26;
constexpr uint8_t SPM3_PIN = 24;
constexpr uint8_t SPM4_PIN = 22;

constexpr uint8_t DCM1_PIN = 44;
constexpr uint8_t DCM2_PIN = 46;

constexpr uint8_t PH_PIN = 36;

constexpr uint8_t SCK_BIT = (1 << PH3);
constexpr uint8_t SDI_BIT = (1 << PH4);
constexpr uint8_t LAT_BIT = (1 << PH5);

void fsout(uint8_t v) {
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

volatile int adc_cache[16] = { 0 };
volatile uint8_t adc_current_pin = 0;

int ar(uint8_t pin) {
	if (pin >= A0) pin -= A0;
	pin &= 0x0F;

	if (!(SREG & (1 << SREG_I))) {
		ADMUX = (1 << REFS0) | (pin & 0x07);
		ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 1) << MUX5);
		ADCSRA |= (1 << ADSC);
		while (ADCSRA & (1 << ADSC));
		return ADC;
	}

	int v;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		v = adc_cache[pin];
	}
	return v;
}

ISR(ADC_vect) {
	adc_cache[adc_current_pin] = ADC;

	adc_current_pin = (adc_current_pin + 1) & 0x0F;

	ADMUX = (1 << REFS0) | (adc_current_pin & 0x07);
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((adc_current_pin >> 3) & 1) << MUX5);
	ADCSRA |= (1 << ADSC);
}

int dr(uint8_t pin) {
	return (*portInputRegister(digitalPinToPort(pin)) & digitalPinToBitMask(pin)) ? HIGH : LOW;
}

void dw(uint8_t pin, uint8_t val) {
	volatile uint8_t* out = portOutputRegister(digitalPinToPort(pin));
	uint8_t mask = digitalPinToBitMask(pin);
	uint8_t s = SREG;
	cli();
	if (val) *out |= mask;
	else *out &= ~mask;
	SREG = s;
}

template <typename T, typename U, typename V>
T clamp(T v, U lo, V hi) {
	return v < lo ? (T)lo : (v > hi ? (T)hi : v);
}

extern "C" void TIMER3_COMPA_vect(void);

// ここから入力系

struct Dch {
	volatile uint8_t* reg;
	uint8_t           mask;
	uint8_t           stable;
	unsigned long     t;
	bool              fired;
};

struct de {
	uint8_t level;
	bool    ltoh;
	bool    htol;
	Dch* c;

	operator int() const {
		return level;
	}

	bool held(uint16_t ms, uint8_t lv = LOW) {
		if (!c) return false;
		if (c->stable != lv) return false;
		if (c->fired) return false;
		if (millis() - c->t >= ms) {
			c->fired = true;
			return true;
		}
		return false;
	}
};

de edgeUpdate(Dch* c, uint8_t raw, uint16_t lock) {
	de r = { c->stable, false, false, c };
	if (raw == c->stable) return r;

	unsigned long now = millis();
	if (now - c->t < lock) return r;

	r.level = raw;
	r.ltoh = (c->stable == LOW);
	r.htol = (c->stable == HIGH);

	c->stable = raw;
	c->t = now;
	c->fired = false;
	return r;
}

// エッジ入力の共通土台
class InEdge {
protected:
	Dch      st;
	uint16_t lock;
	bool     first = true;
	volatile bool fLtoh = false;
	volatile bool fHtol = false;

	virtual uint8_t read() = 0;

	void poll() {
		uint8_t raw = read();
		if (first) {
			first = false;
			st.stable = raw;
			st.t = millis();
			return;
		}
		de r = edgeUpdate(&st, raw, lock);
		if (r.ltoh) fLtoh = true;
		if (r.htol) fHtol = true;
	}

public:
	InEdge(uint16_t lock) : lock(lock) {
		st.reg = nullptr;
		st.mask = 0;
		st.stable = LOW;
		st.t = 0;
		st.fired = false;
	}

	bool ltoh() {
		bool v;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			v = fLtoh;
			fLtoh = false;
		}
		return v;
	}

	bool htol() {
		bool v;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			v = fHtol;
			fHtol = false;
		}
		return v;
	}

	bool level() const {
		return st.stable;
	}

	operator bool() const {
		return st.stable;
	}

	bool held(uint16_t ms, bool lv) {
		bool v = false;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if (st.stable == lv && !st.fired && millis() - st.t >= ms) {
				st.fired = true;
				v = true;
			}
		}
		return v;
	}
};

// デジタル入力
class di : public InEdge {
private:
	volatile uint8_t* reg;
	uint8_t           mask;

	uint8_t read() override {
		return (*reg & mask) ? HIGH : LOW;
	}

	static di* list[8];
	static uint8_t nList;

	friend void TIMER3_COMPA_vect(void);

	static void pollAll() {
		for (uint8_t i = 0; i < nList; i++) list[i]->poll();
	}

public:
	di(uint8_t pin, uint16_t lock = 10)
		: InEdge(lock), reg(portInputRegister(digitalPinToPort(pin))), mask(digitalPinToBitMask(pin)) {
		uint8_t s = SREG;
		cli();
		if (nList < 8) list[nList++] = this;
		SREG = s;
	}
};
di* di::list[8] = {};
uint8_t di::nList = 0;

// アナログ巡回更新の共通土台
class An {
protected:
	virtual void anaPoll() = 0;

	static An* list[8];
	static uint8_t nList;
	static uint8_t idx;

	friend void TIMER3_COMPA_vect(void);

	static void pollTick() {
		if (!nList) return;
		list[idx]->anaPoll();
		if (++idx >= nList) idx = 0;
	}

public:
	An() {
		uint8_t s = SREG;
		cli();
		if (nList < 8) list[nList++] = this;
		SREG = s;
	}
};
An* An::list[8] = {};
uint8_t An::nList = 0;
uint8_t An::idx = 0;

// フォトリフレクタ
class pr : public InEdge, public An {
private:
	uint8_t pin;
	int     th;

	uint8_t read() override {
		return (ar(pin) > th) ? HIGH : LOW;
	}

	void anaPoll() override {
		poll();
	}

public:
	pr(uint8_t pin, int th = 950, uint16_t lock = 10)
		: InEdge(lock), pin(pin), th(th) {
	}
};

constexpr int SOK_N = 4;
constexpr int sokAd[SOK_N] = { 450, 380, 260, 60 };
constexpr int sokMm[SOK_N] = { 40, 150, 300, 500 };

// 測距モジュール
class sok : public An {
private:
	uint8_t pin;
	int     ring[5] = { 0, 0, 0, 0, 0 };
	uint8_t ri = 0;
	uint8_t cnt = 0;
	bool    sat = false;

	void anaPoll() override {
		ring[ri] = ar(pin);
		if (++ri >= 5) ri = 0;
		if (cnt < 5) cnt++;

		int tmp[5];
		uint8_t n = cnt;
		for (uint8_t i = 0; i < n; i++) tmp[i] = ring[i];
		for (uint8_t i = 1; i < n; i++) {
			int t = tmp[i];
			int8_t j = i - 1;
			while (j >= 0 && tmp[j] > t) {
				tmp[j + 1] = tmp[j];
				j--;
			}
			tmp[j + 1] = t;
		}
		int med = tmp[n / 2];

		long ad = med;
		long m;
		if (ad >= sokAd[0]) {
			m = sokMm[0];
		} else if (ad <= sokAd[SOK_N - 1]) {
			m = sokMm[SOK_N - 1];
		} else {
			uint8_t i = 0;
			while (ad < sokAd[i + 1]) i++;
			long da = sokAd[i] - sokAd[i + 1];
			m = sokMm[i] + ((sokAd[i] - ad) * (long)(sokMm[i + 1] - sokMm[i]) + da / 2) / da;
		}
		if (m >= sokMm[SOK_N - 1]) sat = true;
		else if (m < sokMm[SOK_N - 1] - 20) sat = false;
		if (sat) m = sokMm[SOK_N - 1];

		raw = med;
		mm = (int)m;
		cm = m / 10.0f;
	}

public:
	volatile int   raw = 0;
	volatile int   mm = 0;
	volatile float cm = 0;

	sok(uint8_t pin) : pin(pin) {
	}
};

// 半固定抵抗
class vr : public An {
private:
	uint8_t pin;

	void anaPoll() override {
		raw = ar(pin);
	}

public:
	volatile int raw = 0;

	vr(uint8_t pin) : pin(pin) {
	}

	int to(int lo, int hi) {
		long v = raw;
		return (int)(lo + (v * (long)(hi - lo) + 511) / 1023);
	}
};

// ジョイスティック
class joy : public An {
private:
	uint8_t px, py;

	void anaPoll() override {
		x = ar(px);
		y = ar(py);
	}

public:
	volatile int x = 0;
	volatile int y = 0;

	joy(uint8_t px, uint8_t py) : px(px), py(py) {
	}

	int dir(int div, uint8_t rot = 0, bool mirror = false) {
		static const int C = 532;
		long dx = (int)x - C, dy = (int)y - C;
		if (dx * dx + dy * dy < 165000) return -1;
		double th = atan2((double)dx, (double)dy);
		if (mirror) th = -th;
		th += (rot & 3) * (PI / 2);
		return (int)((th + 4 * PI + PI / div) / (2 * PI / div)) % div;
	}
};

// ロータリーエンコーダ
class enc {
private:
	volatile uint8_t* ra;
	volatile uint8_t* rb;
	uint8_t           ma, mb;
	volatile uint8_t  est;
	volatile long     cnt;
	long              last;
	bool              dir;

	static enc* list[4];
	static uint8_t nList;

	friend void TIMER3_COMPA_vect(void);

	void poll() {
		static const uint8_t table[7][4] = {
			{ 0x00, 0x02, 0x04, 0x00 },
			{ 0x00, 0x00, 0x00, 0x00 },
			{ 0x13, 0x02, 0x00, 0x00 },
			{ 0x03, 0x03, 0x03, 0x00 },
			{ 0x26, 0x00, 0x04, 0x00 },
			{ 0x00, 0x00, 0x00, 0x00 },
			{ 0x06, 0x06, 0x06, 0x00 },
		};
		uint8_t a = (*ra & ma) ? 1 : 0;
		uint8_t b = (*rb & mb) ? 1 : 0;
		est = table[est & 0x0f][(a << 1) | b];
		uint8_t dd = est & 0x30;
		int step = (dd == 0x10) ? (dir ? 1 : -1) : (dd == 0x20) ? (dir ? -1 : 1) : 0;
		cnt += step;
	}

	static void pollAll() {
		for (uint8_t i = 0; i < nList; i++) list[i]->poll();
	}

public:
	enc(uint8_t pa, uint8_t pb, bool d = true)
		: ra(portInputRegister(digitalPinToPort(pa))), rb(portInputRegister(digitalPinToPort(pb))),
		ma(digitalPinToBitMask(pa)), mb(digitalPinToBitMask(pb)), est(0), cnt(0), last(0), dir(d) {
		uint8_t s = SREG;
		cli();
		if (nList < 4) list[nList++] = this;
		SREG = s;
	}

	long count() {
		long v;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			v = cnt;
		}
		return v;
	}

	int delta() {
		long v = count();
		int d = (int)(v - last);
		last = v;
		return d;
	}

	long clampTo(long lo, long hi) {
		long v;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			cnt = clamp(cnt, lo, hi);
			v = cnt;
		}
		return v;
	}

	long loopTo(long lo, long hi) {
		long v;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			long span = hi - lo + 1;
			if (span < 1) span = 1;
			cnt = lo + ((cnt - lo) % span + span) % span;
			v = cnt;
		}
		return v;
	}

	void set(long v = 0) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			cnt = v;
			est = 0;
		}
		last = v;
	}
};
enc* enc::list[4] = {};
uint8_t enc::nList = 0;

#define in auto

// フルカラーLEd
constexpr uint8_t G = 0b100;
constexpr uint8_t B = 0b010;
constexpr uint8_t R = 0b001;
constexpr uint8_t GB = 0b110;
constexpr uint8_t GR = 0b101;
constexpr uint8_t BR = 0b011;
constexpr uint8_t GBR = 0b111;

volatile uint8_t ledColor = 0;
volatile uint8_t ledOpacity = 100;

// フルカラーLED
class Led {
private:
	void update() {
		static uint8_t acc = 0;
		uint8_t prev = acc;
		acc += ledOpacity;
		bool on = (acc < prev);

		dw(LED_R_PIN, (on && (ledColor & 1)) ? HIGH : LOW);
		dw(LED_B_PIN, (on && (ledColor & 2)) ? HIGH : LOW);
		dw(LED_G_PIN, (on && (ledColor & 4)) ? HIGH : LOW);
	}

	friend void TIMER3_COMPA_vect(void);

public:
	void operator()(uint8_t color, uint8_t opacity = 100) {
		static uint8_t pc = 255, po = 255;
		opacity = clamp(opacity, 0, 100);
		if (color == pc && opacity == po) return;
		pc = color;
		po = opacity;
		ledOpacity = (uint8_t)(opacity * 255 / 100);
		ledColor = color;
	}
};
Led led;

// 7セグメントLED
constexpr uint8_t seg[16] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x27, 0x7f, 0x6f,
	0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71,
};
constexpr uint8_t alp[26] = {
	0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x3d, 0x76, 0x06, 0x0e,
	0x76, 0x38, 0x37, 0x54, 0x5c, 0x73, 0x67, 0x50, 0x6d, 0x78,
	0x3e, 0x1c, 0x7e, 0x76, 0x6e, 0x5b
};
constexpr uint8_t SEG_DOT = 0x80;
constexpr uint8_t SEG_MINUS = 0x40;
constexpr uint8_t SEG_NONE = 0x00;

class Disp {
private:
	volatile uint8_t segPattern[3] = { 0, 0, 0 };
	volatile uint8_t segOpacity[3] = { 255, 255, 255 };

	static uint8_t toPattern(char c) {
		if (c >= '0' && c <= '9') return seg[c - '0'];
		if (c >= 'a' && c <= 'z') return alp[c - 'a'];
		if (c >= 'A' && c <= 'Z') return alp[c - 'A'];
		if (c == '-') return SEG_MINUS;
		if (c == '.') return SEG_DOT;
		if (c == '_') return SEG_NONE;
		return (uint8_t)c;
	}

	void update() {
		static uint8_t acc[3] = { 0, 0, 0 };

		uint8_t p0 = acc[0];
		acc[0] += segOpacity[0];
		uint8_t a = (acc[0] < p0) ? segPattern[0] : 0;

		uint8_t p1 = acc[1];
		acc[1] += segOpacity[1];
		uint8_t b = (acc[1] < p1) ? segPattern[1] : 0;

		uint8_t p2 = acc[2];
		acc[2] += segOpacity[2];
		uint8_t c = (acc[2] < p2) ? segPattern[2] : 0;

		PORTH &= ~LAT_BIT;
		fsout(a);
		fsout(b);
		fsout(c);
		PORTH |= LAT_BIT;
	}

	friend void TIMER3_COMPA_vect(void);

public:
	Disp& operator()(uint8_t a, uint8_t b, uint8_t c) {
		static int16_t pp[3] = { -1, -1, -1 };
		if (a == pp[0] && b == pp[1] && c == pp[2]) return *this;
		pp[0] = a; pp[1] = b; pp[2] = c;
		segPattern[0] = a;
		segPattern[1] = b;
		segPattern[2] = c;
		return *this;
	}

	Disp& off() {
		return (*this)(0, 0, 0);
	}

	Disp& s(const char* s) {
		return (*this)(toPattern(s[0]), toPattern(s[1]), toPattern(s[2]));
	}

	Disp& n(int x, bool zero = false, bool left = false) {
		bool neg = x < 0;
		if (neg) x = -x;
		if (neg && x > 99) x = 99;
		if (x > 999) x = 999;
		uint8_t nd = (x >= 100) ? 3 : (x >= 10 ? 2 : 1);
		uint8_t content[3];
		uint8_t ci = 0;
		if (neg) content[ci++] = SEG_MINUS;
		if (nd >= 3) content[ci++] = seg[(x / 100) % 10];
		if (nd >= 2) content[ci++] = seg[(x / 10) % 10];
		content[ci++] = seg[x % 10];
		uint8_t len = ci;
		uint8_t g[3] = { 0x00, 0x00, 0x00 };
		if (zero && !neg) {
			g[0] = seg[(x / 100) % 10];
			g[1] = seg[(x / 10) % 10];
			g[2] = seg[x % 10];
		} else if (left) {
			for (uint8_t i = 0; i < len; i++) g[i] = content[i];
		} else {
			uint8_t off = 3 - len;
			for (uint8_t i = 0; i < len; i++) g[off + i] = content[i];
		}
		(*this)(g[0], g[1], g[2]);
		return *this;
	}

	Disp& f(double f, bool zero = false, bool left = false) {
		bool neg = f < 0;
		double a = neg ? -f : f;
		if (neg) {
			bool dot = a < 9.95;
			int v = dot ? (int)(a * 10 + 0.5) : (int)(a + 0.5);
			if (v > 99) v = 99;
			uint8_t p1 = seg[(v / 10) % 10];
			if (dot) p1 |= SEG_DOT;
			(*this)(SEG_MINUS, p1, seg[v % 10]);
			return *this;
		}
		int v, dot;
		if (a < 9.995) {
			v = (int)(a * 100 + 0.5);
			dot = 0;
		} else if (a < 99.95) {
			v = (int)(a * 10 + 0.5);
			dot = 1;
		} else {
			v = (int)(a + 0.5);
			dot = -1;
		}
		if (v > 999) v = 999;
		if (!zero && dot == 0 && v % 10 == 0) {
			v /= 10;
			dot = 1;
		}
		if (dot != 1) {
			uint8_t p0 = seg[(v / 100) % 10];
			uint8_t p1 = seg[(v / 10) % 10];
			if (dot == 0) p0 |= SEG_DOT;
			(*this)(p0, p1, seg[v % 10]);
			return *this;
		}
		uint8_t g0 = seg[(v / 10) % 10] | SEG_DOT;
		uint8_t g1 = seg[v % 10];
		if (v >= 100) {
			(*this)(seg[(v / 100) % 10], g0, g1);
		} else if (left) {
			(*this)(g0, g1, 0x00);
		} else {
			(*this)(0x00, g0, g1);
		}
		return *this;
	}

	Disp& o(uint8_t oa, uint8_t ob = 0xFF, uint8_t oc = 0xFF) {
		if (ob == 0xFF) ob = oa;
		if (oc == 0xFF) oc = ob;
		static int16_t po[3] = { -1, -1, -1 };
		uint8_t v[3] = { oa, ob, oc };
		for (uint8_t i = 0; i < 3; i++) {
			uint8_t x = (uint8_t)(clamp(v[i], 0, 100) * 255 / 100);
			if (x != po[i]) {
				po[i] = x;
				segOpacity[i] = x;
			}
		}
		return *this;
	}
};
Disp dp;

// DCモータ
class Dcm {
public:
	int8_t now = 0;

	void cw(int spd) {
		TCCR5A &= ~_BV(COM5C1);
		dw(DCM1_PIN, LOW);
		TCCR5A |= _BV(COM5A1);
		OCR5A = spd;
		now = (spd > 0) ? 1 : 0;
	}

	void ccw(int spd) {
		TCCR5A &= ~_BV(COM5A1);
		dw(DCM2_PIN, LOW);
		TCCR5A |= _BV(COM5C1);
		OCR5C = spd;
		now = (spd > 0) ? -1 : 0;
	}

	void br() {
		TCCR5A &= ~(_BV(COM5A1) | _BV(COM5C1));
		dw(DCM1_PIN, HIGH);
		dw(DCM2_PIN, HIGH);
		now = 0;
	}

	void fr() {
		TCCR5A &= ~(_BV(COM5A1) | _BV(COM5C1));
		dw(DCM1_PIN, LOW);
		dw(DCM2_PIN, LOW);
		now = 0;
	}
};
Dcm dm;

// ステッピングモータ
class Spm {
private:
	uint8_t ix = 0;

	void phase(uint8_t s) {
		static const uint8_t tbl[4] = { 0b1001, 0b1100, 0b0110, 0b0011 };
		uint8_t b = tbl[s & 3];
		dw(SPM1_PIN, (b & 1) ? HIGH : LOW);
		dw(SPM2_PIN, (b & 2) ? HIGH : LOW);
		dw(SPM3_PIN, (b & 4) ? HIGH : LOW);
		dw(SPM4_PIN, (b & 8) ? HIGH : LOW);
		ix = s & 3;
	}

public:
	void cw() {
		phase((ix + 3) & 3);
	}

	void ccw() {
		phase((ix + 1) & 3);
	}

	void fr() {
		dw(SPM1_PIN, LOW);
		dw(SPM2_PIN, LOW);
		dw(SPM3_PIN, LOW);
		dw(SPM4_PIN, LOW);
	}

	void br() {
		phase(ix);
	}
};
Spm sm;

// 圧電ブザー
class Bz {
private:
	int pref = -1;
	int pret = -1;

public:
	void operator()(int f) {
		if (f == pref) return;
		pref = f;
		tone(BZ_PIN, f);
	}

	void operator()(int f, unsigned long t) {
		if (f == pref && t == pret) return;
		pref = f;
		pret = t;
		tone(BZ_PIN, f, t);
	}

	void off() {
		pref = pret = -1;
		noTone(BZ_PIN);
	}
};
Bz bz;

#ifdef useir
void ir();
#endif

ISR(TIMER3_COMPA_vect) {
	led.update();
	dp.update();
	enc::pollAll();

	static uint8_t acnt = 0;
	if (++acnt >= 10) {
		An::pollTick();
	}

	static uint8_t dcnt = 0;
	if (++dcnt >= 100) {
		dcnt = 0;
		di::pollAll();
	}

#ifdef useir
	ir();
#endif
}

#define L LOW
#define H HIGH

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
	dw(LAT_PIN, HIGH);
	dw(SCK_PIN, LOW);
	dw(SDI_PIN, LOW);

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

	randomSeed(ar(A15) ^ micros());

	led(0);
	dp.off();
	dm.fr();
	sm.fr();
	bz.off();

	ADCSRA = (ADCSRA & ~0x07) | (1 << ADPS2);
	ADCSRA |= (1 << ADIE);

	cli();
	TCCR3A = TCCR3B = TCNT3 = 0;
	OCR3A = F_CPU / 64 / 10000 - 1;
	TCCR3B |= (1 << WGM32);
	TCCR3B |= (1 << CS31) | (1 << CS30);
	TIMSK3 |= (1 << OCIE3A);

	ADMUX = (1 << REFS0);
	ADCSRB &= ~(1 << MUX5);
	ADCSRA |= (1 << ADSC);

	sei();

	delay(100);
}
