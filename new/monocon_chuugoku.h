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

int ar(uint8_t pin) {
	if (pin >= A0) pin -= A0;
	ADMUX = (1 << REFS0) | (pin & 0x07);
	ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 1) << MUX5);
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	return ADC;
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

// ここから入力系 (begin() の後ろに追加)

struct Dch {
	uint8_t           pin;
	volatile uint8_t* reg;
	uint8_t           mask;
	uint8_t           stable;
	unsigned long     t;
	bool              fired;
	bool              init;
};

constexpr uint8_t NCH = 12;
Dch dchPool[NCH] = {};

Dch* dchSlot(uint8_t pin, bool dig, uint8_t raw0 = LOW) {
	Dch* freeCh = nullptr;
	for (uint8_t i = 0; i < NCH; i++) {
		if (dchPool[i].init && dchPool[i].pin == pin) return &dchPool[i];
		if (!dchPool[i].init && !freeCh) freeCh = &dchPool[i];
	}
	if (!freeCh) return nullptr;
	Dch& c = *freeCh;
	c.pin = pin;
	if (dig) {
		c.reg = portInputRegister(digitalPinToPort(pin));
		c.mask = digitalPinToBitMask(pin);
		c.stable = (*c.reg & c.mask) ? HIGH : LOW;
	} else {
		c.reg = nullptr;
		c.mask = 0;
		c.stable = raw0;
	}
	c.t = millis();
	c.fired = false;
	c.init = true;
	return &c;
}

// エッジ・レベル入力の結果型 (デジタル / フォトリフレクタ)
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

// デジタル入力
de di(uint8_t pin, uint16_t lock = 10) {
	Dch* c = dchSlot(pin, true);
	if (!c) return { LOW, false, false, nullptr };
	uint8_t raw = (*c->reg & c->mask) ? HIGH : LOW;
	return edgeUpdate(c, raw, lock);
}

// エンコーダ内部チャネル
struct Ech {
	uint8_t           pa, pb;
	volatile uint8_t* ra;
	uint8_t           ma;
	volatile uint8_t* rb;
	uint8_t           mb;
	uint8_t           est;
	long              c;
	bool              init;
};

constexpr uint8_t NEC = 6;
Ech echPool[NEC] = {};

Ech* echSlot(uint8_t pa, uint8_t pb) {
	Ech* freeCh = nullptr;
	for (uint8_t i = 0; i < NEC; i++) {
		if (echPool[i].init && echPool[i].pa == pa && echPool[i].pb == pb) return &echPool[i];
		if (!echPool[i].init && !freeCh) freeCh = &echPool[i];
	}
	if (!freeCh) return nullptr;
	Ech& e = *freeCh;
	e.pa = pa;
	e.pb = pb;
	e.ra = portInputRegister(digitalPinToPort(pa));
	e.ma = digitalPinToBitMask(pa);
	e.rb = portInputRegister(digitalPinToPort(pb));
	e.mb = digitalPinToBitMask(pb);
	e.est = 0;
	e.c = 0;
	e.init = true;
	return &e;
}

int encPoll(Ech* e, bool dir) {
	uint8_t a = (*e->ra & e->ma) ? 1 : 0;
	uint8_t b = (*e->rb & e->mb) ? 1 : 0;

	static const uint8_t table[7][4] = {
		{ 0x00, 0x02, 0x04, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x13, 0x02, 0x00, 0x00 },
		{ 0x03, 0x03, 0x03, 0x00 },
		{ 0x26, 0x00, 0x04, 0x00 },
		{ 0x00, 0x00, 0x00, 0x00 },
		{ 0x06, 0x06, 0x06, 0x00 },
	};
	e->est = table[e->est & 0x0f][(a << 1) | b];
	uint8_t dd = e->est & 0x30;
	int step = 0;
	if (dd == 0x10)      step = dir ? 1 : -1;
	else if (dd == 0x20) step = dir ? -1 : 1;
	e->c += step;
	return step;
}

// 回転入力の結果型 (エンコーダ)
struct Enc {
	Ech* e;
	bool dir;

	int delta() {
		if (!e) return 0;
		return encPoll(e, dir);
	}

	long count() {
		if (!e) return 0;
		encPoll(e, dir);
		return e->c;
	}

	long clampTo(long lo, long hi) {
		if (!e) return lo;
		encPoll(e, dir);
		e->c = clamp(e->c, lo, hi);
		return e->c;
	}

	long loopTo(long lo, long hi) {
		if (!e) return lo;
		encPoll(e, dir);
		long span = hi - lo + 1;
		if (span < 1) span = 1;
		e->c = lo + ((e->c - lo) % span + span) % span;
		return e->c;
	}

	void set(long v = 0) {
		if (!e) return;
		e->c = v;
		e->est = 0;
	}
};

// ジョイスティック
struct Joy {
	int x, y;

	int dir(int div, uint8_t rot = 0, bool mirror = false) const {
		static const int C = 532;
		long dx = x - C, dy = y - C;
		if (dx * dx + dy * dy < 165000) return -1;
		double th = atan2((double)dx, (double)dy);
		if (mirror) th = -th;
		th += (rot & 3) * (PI / 2);
		return (int)((th + 4 * PI + PI / div) / (2 * PI / div)) % div;
	}
};

// アナログ入力
class An {
public:
	// ロータリーエンコーダ
	Enc enc(uint8_t pa, uint8_t pb, bool dir = true) {
		return { echSlot(pa, pb), dir };
	}

	// フォトリフレクタ
	de pr(uint8_t pin, int th = 950, uint16_t lock = 10) {
		uint8_t raw = (ar(pin) > th) ? HIGH : LOW;
		Dch* c = dchSlot(pin, false, raw);
		if (!c) return { LOW, false, false, nullptr };
		return edgeUpdate(c, raw, lock);
	}

	// 測距モジュール
	int sokRaw(uint8_t pin, uint8_t n = 5) {
		int v[9];
		if (n > 9) n = 9;
		for (uint8_t i = 0; i < n; i++) v[i] = ar(pin);
		for (uint8_t i = 1; i < n; i++) {
			int t = v[i];
			int8_t j = i - 1;
			while (j >= 0 && v[j] > t) {
				v[j + 1] = v[j];
				j--;
			}
			v[j + 1] = t;
		}
		return v[n / 2];
	}

	static constexpr int SOK_N = 4;
	static constexpr int sokAd[SOK_N] = { 450, 380, 260, 60 };
	static constexpr int sokMm[SOK_N] = { 40, 150, 300, 500 };

	float sok(uint8_t pin) {
		long ad = sokRaw(pin);
		long mm;

		if (ad >= sokAd[0]) {
			mm = sokMm[0];
		} else if (ad <= sokAd[SOK_N - 1]) {
			mm = sokMm[SOK_N - 1];
		} else {
			uint8_t i = 0;
			while (ad < sokAd[i + 1]) i++;
			long da = sokAd[i] - sokAd[i + 1];
			mm = sokMm[i] + ((sokAd[i] - ad) * (long)(sokMm[i + 1] - sokMm[i]) + da / 2) / da;
		}

		static bool sat = false;
		if (mm >= sokMm[SOK_N - 1]) sat = true;
		else if (mm < sokMm[SOK_N - 1] - 20) sat = false;
		if (sat) mm = sokMm[SOK_N - 1];

		return mm / 10.0f;
	}

	// 半固定抵抗
	int vr(uint8_t pin, int lo = 0, int hi = 1023) {
		long v = ar(pin);
		return (int)(lo + (v * (long)(hi - lo) + 511) / 1023);
	}

	// ジョイスティック
	Joy joy(uint8_t px, uint8_t py) {
		Joy j;
		j.x = ar(px);
		j.y = ar(py);
		return j;
	}
};
An an;

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

extern "C" void TIMER3_COMPA_vect(void);

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

// 圧電ブザー
void bz(int f) {
	tone(BZ_PIN, f);
}

void bz(int f, unsigned long t) {
	tone(BZ_PIN, f, t);
}

void bzoff() {
	noTone(BZ_PIN);
}

#ifdef useir
void ir();
#endif

ISR(TIMER3_COMPA_vect) {
	led.update();
	dp.update();

#ifdef useir
	ir();
#endif
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


	ADCSRA = (ADCSRA & ~0x07) | (1 << ADPS2);

	cli();
	TCCR3A = TCCR3B = TCNT3 = 0;
	OCR3A = F_CPU / 64 / 10000 - 1;
	TCCR3B |= (1 << WGM32);
	TCCR3B |= (1 << CS31) | (1 << CS30);
	TIMSK3 |= (1 << OCIE3A);
	sei();
}
