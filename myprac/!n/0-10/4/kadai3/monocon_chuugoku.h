/**********************************************/
// 高校生ものづくりコンテスト2026全国大会
//
// 地区名: 中国地区
// 学校名: 岡山県立岡山工業高等学校
// 氏名: 青山 晃大
// 作成年月日: 2026/07/08
/**********************************************/

#pragma once

#include <util/atomic.h>

constexpr uint8_t a0 = A0;
constexpr uint8_t a1 = A1;
constexpr uint8_t a2 = A2;
constexpr uint8_t a3 = A3;
constexpr uint8_t d0 = 10;
constexpr uint8_t d1 = 11;
constexpr uint8_t d2 = 12;
constexpr uint8_t d3 = 13;

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

constexpr uint8_t PH = 36;

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

long clampv(long v, long lo, long hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}

long toward(long cur, long target, long step) {
	if (cur < target) return (cur + step > target) ? target : cur + step;
	if (cur > target) return (cur - step < target) ? target : cur - step;
	return cur;
}

struct Dch {
	uint8_t           pin;
	volatile uint8_t* reg;
	uint8_t           mask;
	uint8_t           stable;
	unsigned long     t;
	bool              fired;
	bool              init;
};

struct de {
	uint8_t level;
	bool    ltoh;
	bool    htol;
	Dch* ch;

	operator int() const {
		return level;
	}

	bool held(unsigned long ms, uint8_t lv = HIGH) {
		if (!ch) return false;
		if (ch->stable != lv) return false;
		if (ch->fired) return false;
		if (millis() - ch->t >= ms) {
			ch->fired = true;
			return true;
		}
		return false;
	}
};

struct Joy {
	int x, y;

	int dir(int div = 4) const {
		long dx = x - 511, dy = y - 511;
		if (dx * dx + dy * dy < 20000) return -1;
		return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / div) / (2 * PI / div)) % div;
	}
};

class In {

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
		freeCh->t = millis();
		freeCh->fired = false;
		freeCh->init = true;
		return freeCh;
	}

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
	static const uint8_t NEC = 4;
	Ech ec[NEC] = {};

	Ech* eslot(uint8_t pa, uint8_t pb) {
		Ech* freeCh = nullptr;
		for (uint8_t i = 0; i < NEC; i++) {
			if (ec[i].init && ec[i].pa == pa && ec[i].pb == pb) return &ec[i];
			if (!ec[i].init && !freeCh) freeCh = &ec[i];
		}
		if (!freeCh) return nullptr;
		freeCh->pa = pa;
		freeCh->pb = pb;
		freeCh->ra = portInputRegister(digitalPinToPort(pa));
		freeCh->ma = digitalPinToBitMask(pa);
		freeCh->rb = portInputRegister(digitalPinToPort(pb));
		freeCh->mb = digitalPinToBitMask(pb);
		freeCh->est = 0;
		freeCh->c = 0;
		freeCh->init = true;
		return freeCh;
	}

public:

	de d(uint8_t pin, uint16_t lock = 10) {
		Dch* c = slot(pin);
		if (!c) return { LOW, false, false, nullptr };
		uint8_t raw = (*c->reg & c->mask) ? HIGH : LOW;
		de r = { c->stable, false, false, c };
		if (raw == c->stable) return r;
		unsigned long now = millis();
		if (now - c->t < lock) return r;
		r.ltoh = (c->stable == LOW && raw == HIGH);
		r.htol = (c->stable == HIGH && raw == LOW);
		r.level = raw;
		c->stable = raw;
		c->t = now;
		c->fired = false;
		return r;
	}

	bool htol(uint8_t pin) {
		return d(pin).htol;
	}

	bool ltoh(uint8_t pin) {
		return d(pin).ltoh;
	}

	bool held(uint8_t pin, unsigned long ms, uint8_t lv = HIGH) {
		Dch* c = slot(pin);
		if (!c) return false;
		if (c->stable != lv) return false;
		if (c->fired) return false;
		if (millis() - c->t >= ms) {
			c->fired = true;
			return true;
		}
		return false;
	}

	Joy joy(uint8_t px, uint8_t py) {
		Joy j;
		j.x = ar(px);
		j.y = ar(py);
		return j;
	}

	int encDelta(uint8_t pa, uint8_t pb, bool dir = true) {
		Ech* e = eslot(pa, pb);
		if (!e) return 0;
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
		if (dd == 0x10)      return dir ? 1 : -1;
		else if (dd == 0x20) return dir ? -1 : 1;
		return 0;
	}

	long enc(uint8_t pa, uint8_t pb, bool dir = true) {
		Ech* e = eslot(pa, pb);
		if (!e) return 0;
		e->c += encDelta(pa, pb, dir);
		return e->c;
	}

	long encClamp(uint8_t pa, uint8_t pb, long lo, long hi, bool dir = true) {
		Ech* e = eslot(pa, pb);
		if (!e) return lo;
		e->c += encDelta(pa, pb, dir);
		e->c = clampv(e->c, lo, hi);
		return e->c;
	}

	long encLoop(uint8_t pa, uint8_t pb, long lo, long hi, bool dir = true) {
		Ech* e = eslot(pa, pb);
		if (!e) return lo;
		e->c += encDelta(pa, pb, dir);
		long span = hi - lo + 1;
		if (span < 1) span = 1;
		e->c = lo + ((e->c - lo) % span + span) % span;
		return e->c;
	}

	void encSet(uint8_t pa, uint8_t pb, long v = 0) {
		Ech* e = eslot(pa, pb);
		if (e) e->c = v;
	}

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

	int sok10(uint8_t pin, int adNear = 450, int adFar = 10,
		int nearMm = 40, int farMm = 500) {
		long ad = sokRaw(pin);
		long den = adNear - adFar;
		if (den == 0) den = 1;

		long mm = farMm + ((ad - adFar) * (long)(nearMm - farMm) + den / 2) / den;
		return (int)clampv(mm, nearMm, farMm) / 1;
	}

	int sok(uint8_t pin, int adNear = 450, int adFar = 10,
		int nearMm = 40, int farMm = 500) {
		return (sok10(pin, adNear, adFar, nearMm, farMm) + 5) / 10;
	}

	bool ref(uint8_t pin, int th, bool writeHigh = false) {
		int v = ar(pin);
		return writeHigh ? (v > th) : (v < th);
	}
};
In in;

const uint8_t seg[16] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66,
	0x6d, 0x7d, 0x27, 0x7f, 0x6f,
	0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71,
};
constexpr uint8_t SEG_DOT = 0x80;
constexpr uint8_t SEG_MINUS = 0x40;

volatile uint8_t segPat[3] = { 0, 0, 0 };
volatile uint8_t segBri[3] = { 255, 255, 255 };

void disp(char a, char b, char c, uint8_t ba = 255, uint8_t bb = 255, uint8_t bc = 255) {
	segPat[0] = (uint8_t)a;
	segBri[0] = ba;
	segPat[1] = (uint8_t)b;
	segBri[1] = bb;
	segPat[2] = (uint8_t)c;
	segBri[2] = bc;
}

void dispRefresh() {
	static uint8_t pc = 0;
	uint8_t a = (((segBri[0] + 8) >> 4) > pc) ? segPat[0] : 0;
	uint8_t b = (((segBri[1] + 8) >> 4) > pc) ? segPat[1] : 0;
	uint8_t c = (((segBri[2] + 8) >> 4) > pc) ? segPat[2] : 0;
	PORTH &= ~LAT_BIT;
	fsout(a);
	fsout(b);
	fsout(c);
	PORTH |= LAT_BIT;
	pc = (pc + 1) & 15;
}

void dispn(int n, bool pad = false) {
	bool neg = n < 0;
	if (neg) n = -n;
	if (neg && n > 99) n = 99;
	if (n > 999) n = 999;
	uint8_t bh = ((pad && !neg) || n >= 100) ? seg[(n / 100) % 10] : 0x00;
	uint8_t bt = ((pad && !neg) || n >= 10) ? seg[(n / 10) % 10] : 0x00;
	uint8_t bo = seg[n % 10];
	if (neg) {
		if (n < 10) bt = SEG_MINUS;
		else        bh = SEG_MINUS;
	}
	disp(bh, bt, bo);
}

void dispn(double f) {
	bool neg = f < 0;
	double a = neg ? -f : f;

	if (neg) {
		if (a < 9.95) {
			int v = (int)(a * 10 + 0.5);
			if (v > 99) v = 99;
			disp(SEG_MINUS, seg[(v / 10) % 10] | SEG_DOT, seg[v % 10]);
		} else {
			int v = (int)(a + 0.5);
			if (v > 99) v = 99;
			disp(SEG_MINUS, seg[(v / 10) % 10], seg[v % 10]);
		}
	} else {
		if (a < 9.995) {
			int v = (int)(a * 100 + 0.5);
			if (v > 999) v = 999;
			disp(seg[(v / 100) % 10] | SEG_DOT, seg[(v / 10) % 10], seg[v % 10]);
		} else if (a < 99.95) {
			int v = (int)(a * 10 + 0.5);
			if (v > 999) v = 999;
			disp(seg[(v / 100) % 10], seg[(v / 10) % 10] | SEG_DOT, seg[v % 10]);
		} else {
			int v = (int)(a + 0.5);
			if (v > 999) v = 999;
			disp(seg[(v / 100) % 10], seg[(v / 10) % 10], seg[v % 10]);
		}
	}
}

void dispn(float f) {
	dispn((double)f);
}

template <typename T>
void dispn(T n, bool pad = false) {
	dispn((int)n, pad);
}

void dispOff() {
	disp(0, 0, 0);
}

void bz(int f) {
	tone(BZ_PIN, f);
}

void bz(int f, unsigned long t) {
	tone(BZ_PIN, f, t);
}

void bzoff() {
	noTone(BZ_PIN);
}

constexpr int nc4 = 262;
constexpr int nd4 = 294;
constexpr int ne4 = 330;
constexpr int nf4 = 349;
constexpr int ng4 = 392;
constexpr int na4 = 440;
constexpr int nb4 = 494;
constexpr int nc5 = 523;
constexpr int nd5 = 587;
constexpr int ne5 = 659;
constexpr int nf5 = 698;
constexpr int ng5 = 784;
constexpr int na5 = 880;
constexpr int nb5 = 988;
constexpr int nc6 = 1047;
constexpr int nr = 0;

class Melody {
	const int* ns = nullptr;
	const int* ds = nullptr;
	int len = 0, idx = 0;
	unsigned long next = 0;
	bool run = false;
	bool rep = false;

public:
	void play(const int* notes, const int* durs, int n, bool repeat = false) {
		ns = notes;
		ds = durs;
		len = n;
		idx = 0;
		run = true;
		next = 0;
		rep = repeat;
	}

	void stop() {
		run = false;
		noTone(BZ_PIN);
	}

	bool playing() {
		return run;
	}

	void update() {
		if (!run) return;
		unsigned long now = millis();
		if ((long)(now - next) < 0) return;

		if (idx >= len) {
			if (rep) {
				idx = 0;
			} else {
				run = false;
				noTone(BZ_PIN);
				return;
			}
		}

		if (ns[idx] > 0) tone(BZ_PIN, ns[idx]);
		else noTone(BZ_PIN);
		next = now + ds[idx];
		idx++;
	}
};
Melody mel;

constexpr uint8_t G = 0b100;
constexpr uint8_t B = 0b010;
constexpr uint8_t R = 0b001;
constexpr uint8_t GB = 0b110;
constexpr uint8_t GR = 0b101;
constexpr uint8_t BR = 0b011;
constexpr uint8_t GBR = 0b111;

volatile uint8_t ledMask = 0;
volatile uint8_t ledBri = 0;

class Led {
public:

	void operator()(uint8_t m, int bri = 100) {
		bri = (int)clampv(bri, 0, 100);
		ledMask = m;
		ledBri = (uint8_t)(bri * 255 / 100);
	}
	void off() {
		ledMask = 0;
		ledBri = 0;
	}
};
Led led;

void ledRefresh() {
	static uint8_t pc = 0;
	bool on = (((ledBri + 8) >> 4) > pc);
	dw(LED_R_PIN, (on && (ledMask & 1)) ? HIGH : LOW);
	dw(LED_B_PIN, (on && (ledMask & 2)) ? HIGH : LOW);
	dw(LED_G_PIN, (on && (ledMask & 4)) ? HIGH : LOW);
	pc = (pc + 1) & 15;
}

const long SPR = 2048;

class Spm {
	volatile int8_t ix = 0;
	volatile long   ps = 0;
	unsigned long stepPre = 0;

	volatile uint16_t ivFast = 0;
	volatile uint16_t ivSlow = 0;
	volatile uint16_t rampN = 0;
	volatile uint16_t ivCur = 0;
	volatile uint16_t isrCnt = 0;

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
	volatile long rem = 0;

	void cw() {
		phase((ix + 3) & 3);
		ps++;
	}

	void ccw() {
		phase((ix + 1) & 3);
		ps--;
	}

	void drive(int spd) {
		if (spd == 0) return;
		unsigned long ms = (spd > 0) ? spd : -spd;
		unsigned long now = millis();
		if (now - stepPre < ms) return;
		stepPre = now;
		if (spd > 0) cw();
		else ccw();
	}

	void off() {
		dw(SPM1_PIN, LOW);
		dw(SPM2_PIN, LOW);
		dw(SPM3_PIN, LOW);
		dw(SPM4_PIN, LOW);
	}

	void mv(long n) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			rem += n;
		}
	}

	void to(long t) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			rem = t - ps;
		}
	}

	void stop() {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			rem = 0;
		}
	}

	void seek(long target) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			long cur = ((ps % SPR) + SPR) % SPR;
			long tgt = ((target % SPR) + SPR) % SPR;
			long d = ((tgt - cur) % SPR + SPR) % SPR;
			if (d > SPR / 2) d -= SPR;
			rem = d;
		}
	}

	void seek(long target, bool cw) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			long cur = ((ps % SPR) + SPR) % SPR;
			long tgt = ((target % SPR) + SPR) % SPR;
			long d = ((tgt - cur) % SPR + SPR) % SPR;
			if (cw)          rem = d;
			else if (d == 0) rem = 0;
			else             rem = d - SPR;
		}
	}

	long divStep(long i, long n = 12) {
		return lround((double)i * SPR / n);
	}

	void stepNow() {
		if (rem > 0) {
			cw();
			rem--;
		} else if (rem < 0) {
			ccw();
			rem++;
		}
	}

	void step(unsigned long ms) {
		if (rem == 0) return;
		unsigned long now = millis();
		if (now - stepPre < ms) return;
		stepPre = now;
		stepNow();
	}

	void run(unsigned long fast, unsigned long slow = 0, uint16_t ramp = 0) {
		if (slow < fast) {
			slow = fast;
			ramp = 0;
		}
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			ivFast = (uint16_t)(fast * 10);
			ivSlow = (uint16_t)(slow * 10);
			rampN = ramp;
			ivCur = ivSlow;
			isrCnt = 0;
		}
	}
	void runOff() {
		ivFast = 0;
	}

	void isrTick() {
		if (ivFast == 0) return;
		if (rem == 0) {
			ivCur = ivSlow;
			return;
		}
		if (++isrCnt < ivCur) return;
		isrCnt = 0;
		stepNow();
		if (rampN == 0) {
			ivCur = ivFast;
			return;
		}

		uint16_t dec = (ivSlow - ivFast) / rampN;
		if (dec == 0) dec = 1;
		uint16_t acc = (ivCur > ivFast + dec) ? ivCur - dec : ivFast;

		long r = (rem >= 0) ? rem : -rem;
		uint16_t brk = ivFast;
		if (r < (long)rampN) {
			brk = ivFast + (uint16_t)(((uint32_t)(ivSlow - ivFast) * (rampN - r)) / rampN);
		}
		ivCur = (acc > brk) ? acc : brk;
	}

	bool moving() {
		long r;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			r = rem;
		}
		return r != 0;
	}

	long pos() {
		long p;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			p = ps;
		}
		return p;
	}

	void zero() {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			ps = 0;
			rem = 0;
		}
	}

	static long degToStep(float deg) {
		return lround(deg * SPR / 360.0);
	}

	static float stepToDeg(long s) {
		return s * 360.0 / SPR;
	}

	void toDeg(float deg) {
		to(degToStep(deg));
	}

	void seekDeg(float deg) {
		seek(degToStep(deg));
	}

	float posDeg() {
		return stepToDeg(pos());
	}
};
Spm sm;

class Dcm {
	int cs = 0;

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

	void drive(int spd) {
		if (spd > 0) {
			cw(spd > 255 ? 255 : spd);
		} else if (spd < 0) {
			ccw(-spd > 255 ? 255 : -spd);
		} else {
			br();
		}
		cs = (int)clampv(spd, -255, 255);
	}

	void ramp(int target, int step) {
		cs = (int)toward(cs, target, step);
		drive(cs);
	}

	int speed() {
		return cs;
	}
};
Dcm dm;

long deadband(long v, long center, long width) {
	return (v > center - width && v < center + width) ? center : v;
}

bool blink(unsigned long ms) {
	return (millis() / ms) & 1;
}

long rnd(long n) {
	return random(n);
}

long rndDiff(long n) {
	static long last = -1;
	long v;
	do {
		v = random(n);
	} while (n > 1 && v == last);
	last = v;
	return v;
}

class iv {
	unsigned long pre = 0;
	unsigned long pausedAt = 0;
	bool paused = false;

public:
	bool operator()(unsigned long ms) {
		if (paused) return false;
		unsigned long now = millis();
		if (now - pre >= ms) {
			pre = now;
			return true;
		}
		return false;
	}
	void reset() {
		pre = millis();
	}

	void wait() {
		if (paused) return;
		pausedAt = millis();
		paused = true;
	}

	void go() {
		if (!paused) return;
		pre += millis() - pausedAt;
		paused = false;
	}

	bool isWait() {
		return paused;
	}
};

class ti {
	unsigned long lim = 0;
	bool run = false;

public:
	void start(unsigned long ms) {
		lim = millis() + ms;
		run = true;
	}

	void stop() {
		run = false;
	}

	bool active() {
		return run;
	}

	bool done() {
		if (run && (long)(millis() - lim) >= 0) {
			run = false;
			return true;
		}
		return false;
	}

	unsigned long remain() {
		if (!run) return 0;
		long r = (long)(lim - millis());
		return r > 0 ? (unsigned long)r : 0;
	}
};

class Sw {
	unsigned long t0 = 0;
	unsigned long fix = 0;
	bool run = false;

public:
	void start() {
		t0 = millis();
		run = true;
	}

	void stop() {
		if (run) {
			fix = millis() - t0;
			run = false;
		}
	}

	void reset() {
		run = false;
		fix = 0;
	}

	bool running() {
		return run;
	}

	unsigned long ms() {
		return run ? millis() - t0 : fix;
	}

	unsigned long operator()() {
		return ms();
	}
};

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
		if (count > 0 && cur >= count) cur = 0;

		pos = 0;
		moved = false;
		exited = false;
	}
	bool on() {
		if (moved) {
			pos++;
			return false;
		}
		return (pos++ == cur);
	}

	bool operator()() {
		return on();
	}

	void next() {
		cur++;
		if (count > 0 && cur >= count) cur = 0;
		t0 = millis();
		fresh = true;
		moved = true;
	}

	void prev() {
		cur--;
		if (cur < 0) cur = (count > 0) ? count - 1 : 0;
		t0 = millis();
		fresh = true;
		moved = true;
	}

	void to(int s) {
		cur = s;
		t0 = millis();
		fresh = true;
		moved = true;
	}

	bool is(int s) {
		return cur == s;
	}

	int now() {
		return cur;
	}

	int steps() {
		return count;
	}

	bool in() {
		if (fresh) {
			fresh = false;
			return true;
		}
		return false;
	}

	bool out() {
		if (moved && !exited) {
			exited = true;
			return true;
		}
		return false;
	}

	unsigned long elapsed() {
		return millis() - t0;
	}

	bool after(unsigned long ms) {
		return millis() - t0 >= ms;
	}
};

#ifdef timer3
void isr();
#endif

ISR(TIMER3_COMPA_vect) {
	dispRefresh();
	ledRefresh();
	sm.isrTick();
#ifdef timer3
	isr();
#endif
}

void begin(void) {
	pinMode(a0, INPUT);
	pinMode(a1, INPUT);
	pinMode(a2, INPUT);
	pinMode(a3, INPUT);
	pinMode(d0, INPUT);
	pinMode(d1, INPUT);
	pinMode(d2, INPUT);
	pinMode(d3, INPUT);

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
	pinMode(PH, INPUT);

	sm.off();
	dm.fr();
	dispOff();

	randomSeed(ar(A15) ^ micros());

	ADCSRA = (ADCSRA & ~0x07) | (1 << ADPS2);

	cli();
	TCCR3A = TCCR3B = TCNT3 = 0;
	OCR3A = F_CPU / 64 / 10000 - 1;
	TCCR3B |= (1 << WGM32);
	TCCR3B |= (1 << CS31) | (1 << CS30);
	TIMSK3 |= (1 << OCIE3A);
	sei();
}
