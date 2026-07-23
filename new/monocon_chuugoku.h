/**********************************************/
// 高校生ものづくりコンテスト2026全国大会
//
// 地区名: 中国地区
// 学校名: 岡山県立岡山工業高等学校
// 氏名: 青山 晃大
// 全国大会・必要最小限自動更新版: 2026/07/15
/**********************************************/

#ifndef MONOCON_CHUUGOKU_H
#define MONOCON_CHUUGOKU_H

#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>

#ifdef useir
void ir();
#endif

constexpr uint8_t a1 = A0;
constexpr uint8_t a2 = A1;
constexpr uint8_t a3 = A2;
constexpr uint8_t a4 = A3;
constexpr uint8_t d1 = 10;
constexpr uint8_t d2 = 11;
constexpr uint8_t d3 = 12;
constexpr uint8_t d4 = 13;

// 7セグ固定配線: D6=PH3, D7=PH4, D8=PH5

constexpr uint8_t SCK_BIT = _BV(PH3);
constexpr uint8_t SDI_BIT = _BV(PH4);
constexpr uint8_t LAT_BIT = _BV(PH5);

constexpr uint8_t G = 0b100;
constexpr uint8_t B = 0b010;
constexpr uint8_t R = 0b001;
constexpr uint8_t GB = 0b110;
constexpr uint8_t GR = 0b101;
constexpr uint8_t BR = 0b011;
constexpr uint8_t GBR = 0b111;

constexpr uint8_t L = LOW;
constexpr uint8_t H = HIGH;

template <typename T, typename U, typename V>
inline T clamp(T v, U lo, V hi) {
	return v < static_cast<T>(lo) ? static_cast<T>(lo)
		: v > static_cast<T>(hi) ? static_cast<T>(hi) : v;
}

extern volatile uint32_t tms;

inline uint32_t monoconMillis() {
	uint32_t v;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { v = tms; }
	return v;
}

inline int monoconAtomicReadInt(volatile const int* p) {
	int v;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { v = *p; }
	return v;
}

// -------------------------------------------------------------
// ADC: ADC完了割り込みで連続スキャン。待機ループは一切使用しない。
// -------------------------------------------------------------
bool adcReg(uint8_t pin, volatile int* dst);
int ar(uint8_t pin);

// -------------------------------------------------------------
// 共通GPIO
// -------------------------------------------------------------
inline int dr(uint8_t pin) {
	const uint8_t port = digitalPinToPort(pin);
	if (port == NOT_A_PIN) return LOW;
	return (*portInputRegister(port) & digitalPinToBitMask(pin)) ? HIGH : LOW;
}

inline void dw(uint8_t pin, uint8_t val) {
	const uint8_t port = digitalPinToPort(pin);
	if (port == NOT_A_PIN) return;
	volatile uint8_t* const out = portOutputRegister(port);
	const uint8_t mask = digitalPinToBitMask(pin);
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		if (val) *out |= mask;
		else     *out &= static_cast<uint8_t>(~mask);
	}
}

namespace monocon_detail {

	extern uint8_t loopEpoch;

	inline uint8_t percentToByte(uint8_t p) {
		if (p >= 100) return 255;
		return static_cast<uint8_t>((static_cast<uint16_t>(p) * 255U + 50U) / 100U);
	}

	inline void shiftBit(uint8_t high) __attribute__((always_inline));
	inline void shiftBit(uint8_t high) {
		if (high) PORTH |= SDI_BIT;
		else      PORTH &= static_cast<uint8_t>(~SDI_BIT);
		PORTH |= SCK_BIT;
		PORTH &= static_cast<uint8_t>(~SCK_BIT);
	}

	// 配線変更なし（D6/D7/D8）のため、8ビットを完全展開して送信する。
	inline void shiftByte(uint8_t v) __attribute__((always_inline));
	inline void shiftByte(uint8_t v) {
		shiftBit(v & 0x80);
		shiftBit(v & 0x40);
		shiftBit(v & 0x20);
		shiftBit(v & 0x10);
		shiftBit(v & 0x08);
		shiftBit(v & 0x04);
		shiftBit(v & 0x02);
		shiftBit(v & 0x01);
	}

	inline void writeDisplay3(uint8_t a, uint8_t b, uint8_t c) {
		PORTH &= static_cast<uint8_t>(~LAT_BIT);
		shiftByte(a);
		shiftByte(b);
		shiftByte(c);
		PORTH |= LAT_BIT;
	}

	inline void compareSwap(int& a, int& b) {
		if (a > b) {
			const int t = a;
			a = b;
			b = t;
		}
	}

	// 固定7比較のmedian-of-five。コピー後の挿入ソートより実行時間が一定で短い。
	inline int median5(int a, int b, int c, int d, int e) {
		compareSwap(a, b);
		compareSwap(c, d);
		if (a > c) {
			const int ta = a; a = c; c = ta;
			const int tb = b; b = d; d = tb;
		}
		compareSwap(b, e);
		compareSwap(b, c);
		compareSwap(d, e);
		compareSwap(c, d);
		return c;
	}

} // namespace monocon_detail

// -------------------------------------------------------------
// 入力エッジ・安定時間デバウンス
// -------------------------------------------------------------
struct Dch {
	uint8_t stable;
	uint8_t candidate;
	uint32_t stableSince;
	uint32_t candidateSince;
	bool candidateActive;
	bool fired;
};

class InEdge {
protected:
	Dch st;
	uint16_t lockMs;
	bool first;
	bool fLtoh;
	bool fHtol;
	uint8_t ltohEpoch;
	uint8_t htolEpoch;

	void pollWith(uint8_t raw, uint32_t now) {
		raw = raw ? HIGH : LOW;

		if (first) {
			first = false;
			st.stable = raw;
			st.candidate = raw;
			st.stableSince = now;
			st.candidateSince = now;
			st.candidateActive = false;
			st.fired = false;
			return;
		}

		if (raw == st.stable) {
			st.candidateActive = false;
			return;
		}

		if (!st.candidateActive || st.candidate != raw) {
			st.candidate = raw;
			st.candidateSince = now;
			st.candidateActive = true;
			if (lockMs != 0) return;
		}

		if (static_cast<uint32_t>(now - st.candidateSince) < lockMs) return;

		const uint8_t old = st.stable;
		st.stable = raw;
		st.stableSince = now;
		st.candidateActive = false;
		st.fired = false;
		if (old == LOW && raw == HIGH) {
			fLtoh = true;
			ltohEpoch = monocon_detail::loopEpoch;
		}
		if (old == HIGH && raw == LOW) {
			fHtol = true;
			htolEpoch = monocon_detail::loopEpoch;
		}
	}

	// 読まれなかったエッジは、検出後に1回の完全なloop()実行機会を
	// 与えた後、自動的に破棄する。
	void expireEdges(uint8_t epoch) {
		if (fLtoh && static_cast<uint8_t>(epoch - ltohEpoch) > 1U) {
			fLtoh = false;
		}
		if (fHtol && static_cast<uint8_t>(epoch - htolEpoch) > 1U) {
			fHtol = false;
		}
	}

public:
	explicit InEdge(uint16_t lock = 10)
		: lockMs(lock), first(true), fLtoh(false), fHtol(false),
		ltohEpoch(0), htolEpoch(0) {
		st.stable = LOW;
		st.candidate = LOW;
		st.stableSince = 0;
		st.candidateSince = 0;
		st.candidateActive = false;
		st.fired = false;
	}

	bool ltoh() {
		const bool v = fLtoh;
		fLtoh = false;
		return v;
	}

	bool htol() {
		const bool v = fHtol;
		fHtol = false;
		return v;
	}

	bool level() const { return st.stable; }
	operator bool() const { return st.stable; }

	bool held(uint16_t ms, bool lv = LOW) {
		const uint32_t now = monoconMillis();
		if (st.stable == static_cast<uint8_t>(lv) && !st.fired &&
			static_cast<uint32_t>(now - st.stableSince) >= ms) {
			st.fired = true;
			return true;
		}
		return false;
	}
};

class di : public InEdge {
private:
	volatile uint8_t* reg;
	uint8_t mask;
	static di* list[8];
	static uint8_t nList;

public:
	explicit di(uint8_t pin, uint16_t lock = 10)
		: InEdge(lock),
		reg(portInputRegister(digitalPinToPort(pin))),
		mask(digitalPinToBitMask(pin)) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if (nList < 8) list[nList++] = this;
		}
	}

	static void serviceAll(uint32_t now) {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) {
			di* const p = list[i];
			p->pollWith((*p->reg & p->mask) ? HIGH : LOW, now);
		}
	}

	static void expireAll(uint8_t epoch) {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) list[i]->expireEdges(epoch);
	}
};

class pr : public InEdge {
private:
	int th;
	volatile int _raw;
	static pr* list[8];
	static uint8_t nList;

public:
	explicit pr(uint8_t pin, int threshold = 950, uint16_t lock = 10)
		: InEdge(lock), th(threshold), _raw(0) {
		adcReg(pin, &_raw);
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if (nList < 8) list[nList++] = this;
		}
	}

	static void serviceAll(uint32_t now) {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) {
			pr* const p = list[i];
			const int value = monoconAtomicReadInt(&p->_raw);
			p->pollWith(value > p->th ? HIGH : LOW, now);
		}
	}

	static void expireAll(uint8_t epoch) {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) list[i]->expireEdges(epoch);
	}
};

constexpr int SOK_N = 4;
constexpr int sokAd[SOK_N] = { 450, 380, 260, 60 };
constexpr int sokMm[SOK_N] = { 40, 150, 300, 500 };
constexpr int32_t slopeQ8[3] = {
	(static_cast<int32_t>(sokMm[1] - sokMm[0]) << 8) / (sokAd[0] - sokAd[1]),
	(static_cast<int32_t>(sokMm[2] - sokMm[1]) << 8) / (sokAd[1] - sokAd[2]),
	(static_cast<int32_t>(sokMm[3] - sokMm[2]) << 8) / (sokAd[2] - sokAd[3])
};

class sok {
private:
	int ring[5];
	uint8_t ri;
	uint8_t sampleCount;
	static sok* list[2];
	static uint8_t nList;

	void serviceOne() {
		const int sample = monoconAtomicReadInt(&_raw);
		ring[ri] = sample;
		if (++ri == 5) ri = 0;
		if (sampleCount < 5) ++sampleCount;
		if (sampleCount < 5) return;

		const int med = monocon_detail::median5(
			ring[0], ring[1], ring[2], ring[3], ring[4]);
		if (med == raw) return;

		raw = med;
		int32_t m;
		if (med >= sokAd[0]) {
			m = sokMm[0];
		} else if (med <= sokAd[SOK_N - 1]) {
			m = sokMm[SOK_N - 1];
		} else {
			uint8_t i = 0;
			while (i < SOK_N - 2 && med < sokAd[i + 1]) ++i;
			m = sokMm[i] +
				((static_cast<int32_t>(sokAd[i] - med) * slopeQ8[i] + 128) >> 8);
		}
		if (m < sokMm[0]) m = sokMm[0];
		if (m > sokMm[SOK_N - 1]) m = sokMm[SOK_N - 1];
		mm = static_cast<int>(m);
		cm = static_cast<float>(m) * 0.1f; // ISR外でのみ計算
	}

public:
	volatile int _raw;
	int raw;
	int mm;
	float cm;

	explicit sok(uint8_t pin)
		: ri(0), sampleCount(0), _raw(0), raw(0), mm(0), cm(0.0f) {
		for (uint8_t i = 0; i < 5; ++i) ring[i] = 0;
		adcReg(pin, &_raw);
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if (nList < 2) list[nList++] = this;
		}
	}

	static void serviceAll() {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) list[i]->serviceOne();
	}
};

class vr {
public:
	volatile int raw;

	explicit vr(uint8_t pin) : raw(0) { adcReg(pin, &raw); }

	int to(int lo, int hi) const {
		const int v = monoconAtomicReadInt(&raw);
		const int32_t span = static_cast<int32_t>(hi) - static_cast<int32_t>(lo);
		return static_cast<int>(static_cast<int32_t>(lo) +
			(static_cast<int32_t>(v) * span + 511L) / 1023L);
	}
};

class joy {
public:
	volatile int x;
	volatile int y;

	joy(uint8_t px, uint8_t py) : x(0), y(0) {
		adcReg(px, &x);
		adcReg(py, &y);
	}

	int dir(int div, uint8_t rot = 0, bool mirror = false) const {
		if (div <= 0) return -1;

		int vx, vy;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			vx = x;
			vy = y;
		}

		const int32_t dx = static_cast<int32_t>(vx) - 532;
		const int32_t dy = static_cast<int32_t>(vy) - 532;
		if (dx * dx + dy * dy < 165000L) return -1;

		// 使用頻度の高い4/8方向は浮動小数点とatan2を使わない。
		if (div == 4) {
			const int32_t ax = dx < 0 ? -dx : dx;
			const int32_t ay = dy < 0 ? -dy : dy;
			int d;
			if (ay >= ax) d = dy >= 0 ? 0 : 2;
			else          d = dx >= 0 ? 1 : 3;
			if (mirror) d = (4 - d) & 3;
			return (d + (rot & 3)) & 3;
		}

		if (div == 8) {
			const int32_t ax = dx < 0 ? -dx : dx;
			const int32_t ay = dy < 0 ? -dy : dy;
			int d;
			// tan(22.5°) ≈ 0.414214, 1/tan ≈ 2.414214
			if (ay * 1000L >= ax * 2414L) {
				d = dy >= 0 ? 0 : 4;
			} else if (ax * 1000L >= ay * 2414L) {
				d = dx >= 0 ? 2 : 6;
			} else if (dx >= 0) {
				d = dy >= 0 ? 1 : 3;
			} else {
				d = dy >= 0 ? 7 : 5;
			}
			if (mirror) d = (8 - d) & 7;
			return (d + ((rot & 3) << 1)) & 7;
		}

		// 任意分割数だけ互換性のためatan2を使用。ISR外なのでブザーには影響しない。
		double th = atan2(static_cast<double>(dx), static_cast<double>(dy));
		if (mirror) th = -th;
		th += (rot & 3) * (PI / 2.0);
		int result = static_cast<int>((th + 4.0 * PI + PI / div) /
			(2.0 * PI / div)) % div;
		if (result < 0) result += div;
		return result;
	}
};

// -------------------------------------------------------------
// ロータリーエンコーダ: 10 kHz Timer1 ISR内は入力・FSM・有効時加算のみ。
// -------------------------------------------------------------
class enc {
private:
	volatile uint8_t* ra;
	volatile uint8_t* rb;
	uint8_t ma;
	uint8_t mb;
	bool samePort;
	volatile uint8_t est;
	volatile int32_t cnt;
	int32_t last;
	bool direction;

	static enc* list[4];
	static uint8_t nList;

	inline void poll() {
		static const uint8_t table[7][4] = {
			{0x00, 0x02, 0x04, 0x00},
			{0x00, 0x00, 0x00, 0x00},
			{0x13, 0x02, 0x00, 0x00},
			{0x03, 0x03, 0x03, 0x00},
			{0x26, 0x00, 0x04, 0x00},
			{0x00, 0x00, 0x00, 0x00},
			{0x06, 0x06, 0x06, 0x00}
		};

		uint8_t a;
		uint8_t b;
		if (samePort) {
			const uint8_t p = *ra;
			a = (p & ma) ? 1 : 0;
			b = (p & mb) ? 1 : 0;
		} else {
			a = (*ra & ma) ? 1 : 0;
			b = (*rb & mb) ? 1 : 0;
		}

		const uint8_t next = table[est & 0x0F][(a << 1) | b];
		est = next;
		const uint8_t event = next & 0x30;
		int8_t step = 0;
		if (event == 0x10) step = direction ? 1 : -1;
		else if (event == 0x20) step = direction ? -1 : 1;
		if (step != 0) cnt += step;
	}

public:
	enc(uint8_t pa, uint8_t pb, bool d = true)
		: ra(portInputRegister(digitalPinToPort(pa))),
		rb(portInputRegister(digitalPinToPort(pb))),
		ma(digitalPinToBitMask(pa)), mb(digitalPinToBitMask(pb)),
		samePort(ra == rb), est(0), cnt(0), last(0), direction(d) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if (nList < 4) list[nList++] = this;
		}
	}

	static inline void isrPollAll() {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) list[i]->poll();
	}

	int32_t count() const {
		int32_t v;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { v = cnt; }
		return v;
	}

	int delta() {
		const int32_t v = count();
		const int32_t diff = v - last;
		last = v;
		if (diff > INT_MAX) return INT_MAX;
		if (diff < INT_MIN) return INT_MIN;
		return static_cast<int>(diff);
	}

	int32_t clampTo(int32_t lo, int32_t hi) {
		if (hi < lo) {
			const int32_t t = lo; lo = hi; hi = t;
		}
		for (;;) {
			const int32_t before = count();
			const int32_t after = clamp(before, lo, hi);
			bool committed = false;
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				if (cnt == before) {
					cnt = after;
					committed = true;
				}
			}
			if (committed) return after;
		}
	}

	int32_t loopTo(int32_t lo, int32_t hi) {
		if (hi < lo) {
			const int32_t t = lo; lo = hi; hi = t;
		}
		for (;;) {
			const int32_t before = count();
			const int64_t span = static_cast<int64_t>(hi) -
				static_cast<int64_t>(lo) + 1LL;
			int64_t r = (static_cast<int64_t>(before) - lo) % span;
			if (r < 0) r += span;
			const int32_t after = static_cast<int32_t>(static_cast<int64_t>(lo) + r);
			bool committed = false;
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				if (cnt == before) {
					cnt = after;
					committed = true;
				}
			}
			if (committed) return after;
		}
	}

	void set(int32_t v = 0) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			cnt = v;
			est = 0;
			last = v;
		}
	}
};

// -------------------------------------------------------------
// フルカラーLED: PWM計算・ポート更新はISR外。
// -------------------------------------------------------------
class Led {
private:
	uint8_t color;
	uint8_t opacity;
	uint8_t acc;
	uint8_t previousState;

	void writeState(uint8_t state) {
		uint8_t pe = PORTE & static_cast<uint8_t>(~(_BV(PE4) | _BV(PE5)));
		if (state & B) pe |= _BV(PE5);
		if (state & G) pe |= _BV(PE4);
		PORTE = pe;

		if (state & R) PORTG |= _BV(PG5);
		else           PORTG &= static_cast<uint8_t>(~_BV(PG5));
	}

public:
	Led() : color(0), opacity(255), acc(0), previousState(0xFF) {}

	void operator()(uint8_t newColor, uint8_t opacityPercent = 100) {
		newColor &= 0x07;
		const uint8_t newOpacity = monocon_detail::percentToByte(
			clamp<uint8_t, int, int>(opacityPercent, 0, 100));
		if (color == newColor && opacity == newOpacity) return;
		color = newColor;
		opacity = newOpacity;
	}

	void serviceTick() {
		uint8_t state;
		if (opacity == 0 || color == 0) {
			state = 0;
		} else if (opacity == 255) {
			state = color; // 真の100%。周期的な1 ms消灯を発生させない。
		} else {
			const uint8_t old = acc;
			acc = static_cast<uint8_t>(acc + opacity);
			state = (acc < old) ? color : 0;
		}
		if (state != previousState) {
			writeState(state);
			previousState = state;
		}
	}
};

constexpr uint8_t seg[16] = {
	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x27,
	0x7F, 0x6F, 0x77, 0x7C, 0x58, 0x5E, 0x79, 0x71
};
constexpr uint8_t alp[26] = {
	0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71, 0x3D, 0x76, 0x06, 0x0E,
	0x76, 0x38, 0x37, 0x54, 0x5C, 0x73, 0x67, 0x50, 0x6D, 0x78,
	0x3E, 0x1C, 0x7E, 0x76, 0x6E, 0x5B
};
constexpr uint8_t SEG_DOT = 0x80;
constexpr uint8_t SEG_MINUS = 0x40;
constexpr uint8_t SEG_NONE = 0x00;

class Disp {
private:
	uint8_t pattern[3];
	uint8_t opacity[3];
	uint8_t acc[3];
	uint8_t previous[3];

	static uint8_t toPattern(char c) {
		if (c >= '0' && c <= '9') return seg[c - '0'];
		if (c >= 'a' && c <= 'z') return alp[c - 'a'];
		if (c >= 'A' && c <= 'Z') return alp[c - 'A'];
		if (c == '-') return SEG_MINUS;
		if (c == '.') return SEG_DOT;
		if (c == '_') return SEG_NONE;
		return static_cast<uint8_t>(c);
	}

public:
	Disp() {
		for (uint8_t i = 0; i < 3; ++i) {
			pattern[i] = 0;
			opacity[i] = 255;
			acc[i] = 0;
			previous[i] = 0xFF;
		}
	}

	Disp& operator()(uint8_t a, uint8_t b, uint8_t c) {
		if (pattern[0] == a && pattern[1] == b && pattern[2] == c) return *this;
		pattern[0] = a;
		pattern[1] = b;
		pattern[2] = c;
		return *this;
	}

	Disp& off() { return (*this)(0, 0, 0); }

	Disp& s(const char* text) {
		if (!text) return off();
		return (*this)(toPattern(text[0]),
			text[1] ? toPattern(text[1]) : 0,
			text[1] && text[2] ? toPattern(text[2]) : 0);
	}

	Disp& n(int x, bool zero = false, bool left = false) {
		int32_t value = x;
		const bool neg = value < 0;
		if (neg) value = -value;
		if (neg && value > 99) value = 99;
		if (value > 999) value = 999;

		const uint8_t nd = value >= 100 ? 3 : value >= 10 ? 2 : 1;
		uint8_t content[4];
		uint8_t ci = 0;
		if (neg) content[ci++] = SEG_MINUS;
		if (nd >= 3) content[ci++] = seg[(value / 100) % 10];
		if (nd >= 2) content[ci++] = seg[(value / 10) % 10];
		content[ci++] = seg[value % 10];

		uint8_t out[3] = { 0, 0, 0 };
		if (zero && !neg) {
			out[0] = seg[(value / 100) % 10];
			out[1] = seg[(value / 10) % 10];
			out[2] = seg[value % 10];
		} else if (left) {
			for (uint8_t i = 0; i < ci && i < 3; ++i) out[i] = content[i];
		} else {
			const uint8_t start = ci < 3 ? static_cast<uint8_t>(3 - ci) : 0;
			const uint8_t source = ci > 3 ? static_cast<uint8_t>(ci - 3) : 0;
			for (uint8_t i = source; i < ci; ++i) out[start + i - source] = content[i];
		}
		return (*this)(out[0], out[1], out[2]);
	}

	Disp& f(double f, bool zero = false, bool left = false) {
		const bool neg = f < 0;
		const double a = neg ? -f : f;
		if (neg) {
			const bool dot = a < 9.95;
			int v = dot ? static_cast<int>(a * 10.0 + 0.5)
				: static_cast<int>(a + 0.5);
			if (v > 99) v = 99;
			uint8_t p1 = seg[(v / 10) % 10];
			if (dot) p1 |= SEG_DOT;
			return (*this)(SEG_MINUS, p1, seg[v % 10]);
		}

		int v;
		int8_t dot;
		if (a < 9.995) {
			v = static_cast<int>(a * 100.0 + 0.5);
			dot = 0;
		} else if (a < 99.95) {
			v = static_cast<int>(a * 10.0 + 0.5);
			dot = 1;
		} else {
			v = static_cast<int>(a + 0.5);
			dot = -1;
		}
		if (v > 999) v = 999;
		if (!zero && dot == 0 && v % 10 == 0) {
			v /= 10;
			dot = 1;
		}
		if (dot != 1) {
			uint8_t p0 = seg[(v / 100) % 10];
			const uint8_t p1 = seg[(v / 10) % 10];
			if (dot == 0) p0 |= SEG_DOT;
			return (*this)(p0, p1, seg[v % 10]);
		}

		const uint8_t p0 = seg[(v / 10) % 10] | SEG_DOT;
		const uint8_t p1 = seg[v % 10];
		if (v >= 100) return (*this)(seg[(v / 100) % 10], p0, p1);
		if (left) return (*this)(p0, p1, 0);
		return (*this)(0, p0, p1);
	}

	Disp& o(uint8_t oa, uint8_t ob = 0xFF, uint8_t oc = 0xFF) {
		if (ob == 0xFF) ob = oa;
		if (oc == 0xFF) oc = ob;
		const uint8_t values[3] = { oa, ob, oc };
		for (uint8_t i = 0; i < 3; ++i) {
			const uint8_t p = clamp<uint8_t, int, int>(values[i], 0, 100);
			opacity[i] = monocon_detail::percentToByte(p);
		}
		return *this;
	}

	void serviceTick() {
		uint8_t out[3];
		for (uint8_t i = 0; i < 3; ++i) {
			if (opacity[i] == 0 || pattern[i] == 0) {
				out[i] = 0;
			} else if (opacity[i] == 255) {
				out[i] = pattern[i];
			} else {
				const uint8_t old = acc[i];
				acc[i] = static_cast<uint8_t>(acc[i] + opacity[i]);
				out[i] = acc[i] < old ? pattern[i] : 0;
			}
		}

		if (out[0] != previous[0] || out[1] != previous[1] || out[2] != previous[2]) {
			monocon_detail::writeDisplay3(out[0], out[1], out[2]);
			previous[0] = out[0];
			previous[1] = out[1];
			previous[2] = out[2];
		}
	}
};

// -------------------------------------------------------------
// DCモータ: Timer5設定をbegin()内で明示し、入力値を0..255に制限。
// -------------------------------------------------------------
class Dcm {
public:
	int8_t now;

	Dcm() : now(0) {}

	void cw(int spd) {
		const uint8_t pwm = static_cast<uint8_t>(clamp<int, int, int>(spd, 0, 255));
		if (pwm == 0) { fr(); return; }
		TCCR5A &= static_cast<uint8_t>(~(_BV(COM5A1) | _BV(COM5C1)));
		PORTL &= static_cast<uint8_t>(~(_BV(PL5) | _BV(PL3)));
		OCR5A = pwm;
		TCCR5A |= _BV(COM5A1);
		now = 1;
	}

	void ccw(int spd) {
		const uint8_t pwm = static_cast<uint8_t>(clamp<int, int, int>(spd, 0, 255));
		if (pwm == 0) { fr(); return; }
		TCCR5A &= static_cast<uint8_t>(~(_BV(COM5A1) | _BV(COM5C1)));
		PORTL &= static_cast<uint8_t>(~(_BV(PL5) | _BV(PL3)));
		OCR5C = pwm;
		TCCR5A |= _BV(COM5C1);
		now = -1;
	}

	void br() {
		TCCR5A &= static_cast<uint8_t>(~(_BV(COM5A1) | _BV(COM5C1)));
		PORTL |= _BV(PL5) | _BV(PL3);
		now = 0;
	}

	void fr() {
		TCCR5A &= static_cast<uint8_t>(~(_BV(COM5A1) | _BV(COM5C1)));
		PORTL &= static_cast<uint8_t>(~(_BV(PL5) | _BV(PL3)));
		now = 0;
	}
};

constexpr uint8_t spmPhaseMask[4] = {
	static_cast<uint8_t>(_BV(PA6) | _BV(PA0)),
	static_cast<uint8_t>(_BV(PA6) | _BV(PA4)),
	static_cast<uint8_t>(_BV(PA4) | _BV(PA2)),
	static_cast<uint8_t>(_BV(PA2) | _BV(PA0))
};
constexpr uint8_t SPM_MASK = _BV(PA6) | _BV(PA4) | _BV(PA2) | _BV(PA0);

class Spm {
private:
	uint8_t ix;

	void phase(uint8_t s) {
		ix = s & 3;
		PORTA = static_cast<uint8_t>((PORTA & ~SPM_MASK) | spmPhaseMask[ix]);
	}

public:
	Spm() : ix(0) {}
	void cw() { phase((ix + 3) & 3); }
	void ccw() { phase((ix + 1) & 3); }
	void fr() { PORTA &= static_cast<uint8_t>(~SPM_MASK); }
	void br() { phase(ix); }
};

// -------------------------------------------------------------
// 圧電ブザー: Timer3 OC3Aハードウェア出力。
// 周波数変更時はタイマー停止→出力切断→TCNT/OCR更新→既知位相で再開。
// CTCの「新TOPが現在TCNTより小さいと比較一致を逃す」問題を回避する。
// -------------------------------------------------------------
class Bz {
private:
	int continuousFrequency;
	volatile uint32_t remainingMs;
	volatile bool timedActive;

	static uint16_t topForFrequency(int f) {
		if (f < 1) f = 1;
		uint32_t clocks = (1000000UL + static_cast<uint32_t>(f) / 2UL) /
			static_cast<uint32_t>(f);
		if (clocks < 1UL) clocks = 1UL;
		if (clocks > 65536UL) clocks = 65536UL;
		return static_cast<uint16_t>(clocks - 1UL);
	}

	void start(int f, uint32_t durationMs, bool timed) {
		if (f <= 0) {
			off();
			return;
		}

		// 32ビット除算は割り込みを許可した状態で済ませる。
		const uint16_t top = topForFrequency(f);

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			// Timer3停止。OC3A切断後、ピンをLOWへ固定。
			TCCR3B = _BV(WGM32);
			TCCR3A &= static_cast<uint8_t>(~_BV(COM3A0));
			PORTE &= static_cast<uint8_t>(~_BV(PE3));

			TCNT3 = 0;
			OCR3A = top;
			TIFR3 = _BV(OCF3A);

			remainingMs = durationMs;
			timedActive = timed && durationMs != 0;

			TCCR3A |= _BV(COM3A0);
			TCCR3B = _BV(WGM32) | _BV(CS31); // /8、OC3Aトグル
		}
	}

	inline void stopFromIsr() {
		TCCR3B = _BV(WGM32);
		TCCR3A &= static_cast<uint8_t>(~_BV(COM3A0));
		PORTE &= static_cast<uint8_t>(~_BV(PE3));
		timedActive = false;
		remainingMs = 0;
	}

public:
	Bz() : continuousFrequency(-1), remainingMs(0), timedActive(false) {}

	void operator()(int f) {
		if (f <= 0) { off(); return; }
		if (!timedActive && f == continuousFrequency) return;
		continuousFrequency = f;
		start(f, 0, false);
	}

	void operator()(int f, uint32_t durationMs) {
		if (durationMs == 0) {
			(*this)(f);
			return;
		}
		// 同じ周波数・時間でも毎回必ず再トリガーする。
		continuousFrequency = -1;
		start(f, durationMs, true);
	}

	void off() {
		continuousFrequency = -1;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			stopFromIsr();
		}
	}

	inline void isrTick() {
		if (!timedActive) return;
		if (remainingMs > 0 && --remainingMs == 0) stopFromIsr();
	}
};

extern Led led;
extern Disp dp;
extern Dcm dm;
extern Spm sm;
extern Bz bz;

void begin();

class Seq {
private:
	int current_ = 0;
	int position_ = 0;
	int count_ = 0;

	uint32_t enteredAt_ = 0;
	uint8_t lastEpoch_ = 0xFF;

	bool initialized_ = false;
	bool moved_ = false;
	bool entryPending_ = true;
	bool exitPending_ = false;

	// 新しいloop()に入ったことを自動検出する
	void syncLoop() {
		const uint8_t epoch = monocon_detail::loopEpoch;

		if (lastEpoch_ == epoch) {
			return;
		}

		// 前回のloop()で記述されていた状態数を記録
		if (position_ > count_) {
			count_ = position_;
		}

		// 状態番号が範囲外なら状態0へ戻す
		if (count_ > 0 && current_ >= count_) {
			current_ = 0;
		}

		position_ = 0;
		moved_ = false;
		exitPending_ = false;
		lastEpoch_ = epoch;

		if (!initialized_) {
			enteredAt_ = millis();
			initialized_ = true;
		}
	}

	void moveTo(int state) {
		syncLoop();

		if (state < 0) {
			state = 0;
		}

		// 状態数が判明している場合だけ上限を制限
		if (count_ > 0 && state >= count_) {
			state = count_ - 1;
		}

		// 同じ状態を指定した場合は何もしない
		if (state == current_) {
			return;
		}

		current_ = state;
		enteredAt_ = millis();

		entryPending_ = true;
		exitPending_ = true;
		moved_ = true;
	}

public:
	Seq() = default;

	Seq(const Seq&) = delete;
	Seq& operator=(const Seq&) = delete;

	// 現在の状態に対応する場所だけtrue
	bool on() {
		syncLoop();

		// 同じloop()内で遷移した後は、後続状態を実行しない
		if (moved_) {
			++position_;
			return false;
		}

		return position_++ == current_;
	}

	bool operator()() {
		return on();
	}

	explicit operator bool() {
		return on();
	}

	// 次の状態へ進む
	void next() {
		syncLoop();

		int target = current_ + 1;

		if (count_ > 0 && target >= count_) {
			target = 0;
		}

		moveTo(target);
	}

	// 前の状態へ戻る
	void prev() {
		syncLoop();

		int target = current_ - 1;

		if (target < 0) {
			target = count_ > 0 ? count_ - 1 : 0;
		}

		moveTo(target);
	}

	// 指定した状態へ移動する
	void to(int state) {
		moveTo(state);
	}

	// 現在の状態を最初からやり直す
	void restart() {
		syncLoop();

		enteredAt_ = millis();
		entryPending_ = true;
		exitPending_ = false;
		moved_ = true;
	}

	// 指定した状態か確認する
	bool is(int state) {
		syncLoop();
		return current_ == state;
	}

	// 現在の状態番号
	int now() {
		syncLoop();
		return current_;
	}

	// 状態数
	int steps() {
		syncLoop();
		return count_;
	}

	// 状態に入った直後に1回だけtrue
	bool in() {
		syncLoop();

		if (!entryPending_) {
			return false;
		}

		entryPending_ = false;
		return true;
	}

	// 状態を移動した直後に1回だけtrue
	bool out() {
		syncLoop();

		if (!exitPending_) {
			return false;
		}

		exitPending_ = false;
		return true;
	}

	// 状態に入ってからの経過時間
	uint32_t elapsed() {
		syncLoop();
		return static_cast<uint32_t>(millis() - enteredAt_);
	}

	// 指定時間が経過したか
	bool after(uint32_t ms) {
		return elapsed() >= ms;
	}
};

// -------------------------------------------------------------
// 一定間隔タイマー: if (timer(100)) { ... }
// wait()/go()で停止・再開しても周期の残り時間を維持する。
// -------------------------------------------------------------
class Iv {
private:
	uint32_t previous_ = 0;
	uint32_t pausedAt_ = 0;
	bool paused_ = false;

public:
	bool operator()(uint32_t ms) {
		if (paused_) return false;
		const uint32_t now = millis();
		if (static_cast<uint32_t>(now - previous_) < ms) return false;
		previous_ = now;
		return true;
	}

	void reset() {
		previous_ = millis();
	}

	void wait() {
		if (paused_) return;
		pausedAt_ = millis();
		paused_ = true;
	}

	void go() {
		if (!paused_) return;
		const uint32_t now = millis();
		previous_ += static_cast<uint32_t>(now - pausedAt_);
		paused_ = false;
	}

	bool isWait() const {
		return paused_;
	}
};

// -------------------------------------------------------------
// ワンショットタイマー: start(ms)後、done()が1回だけtrue。
// 安全に扱える最大時間は約24.8日。
// -------------------------------------------------------------
class Ti {
private:
	uint32_t deadline_ = 0;
	bool running_ = false;

public:
	void start(uint32_t ms) {
		if (ms > 0x7FFFFFFFUL) ms = 0x7FFFFFFFUL;
		deadline_ = millis() + ms;
		running_ = true;
	}

	void stop() {
		running_ = false;
	}

	bool active() const {
		return running_;
	}

	bool done() {
		if (!running_) return false;
		const uint32_t now = millis();
		if (static_cast<int32_t>(now - deadline_) < 0) return false;
		running_ = false;
		return true;
	}

	uint32_t remain() const {
		if (!running_) return 0;
		const int32_t remaining = static_cast<int32_t>(deadline_ - millis());
		return remaining > 0 ? static_cast<uint32_t>(remaining) : 0;
	}
};

// -------------------------------------------------------------
// ストップウォッチ: start()/stop()/reset()、経過時間は sw()。
// -------------------------------------------------------------
class Sw {
private:
	uint32_t startedAt_ = 0;
	uint32_t fixed_ = 0;
	bool running_ = false;

public:
	void start() {
		startedAt_ = millis();
		fixed_ = 0;
		running_ = true;
	}

	void stop() {
		if (!running_) return;
		fixed_ = static_cast<uint32_t>(millis() - startedAt_);
		running_ = false;
	}

	void reset() {
		running_ = false;
		fixed_ = 0;
	}

	bool running() const {
		return running_;
	}

	uint32_t ms() const {
		return running_
			? static_cast<uint32_t>(millis() - startedAt_)
			: fixed_;
	}

	uint32_t operator()() const {
		return ms();
	}

	operator uint32_t() const {
		return ms();
	}
};

// 内部実装

namespace monocon_detail {

	void service();

	struct AdcSlot {
		uint8_t admux;
		uint8_t mux5;
		uint8_t channel;
		volatile int* dst;
	};

	AdcSlot adcSlots[16];
	volatile uint8_t adcCount = 0;
	volatile uint8_t adcIndex = 0;
	volatile bool adcRunning = false;
	bool adcHardwareReady = false;
	volatile int adcFallback[16] = { 0 };
	volatile uint8_t serviceDue = 0;
	uint8_t loopEpoch = 0;

	inline uint8_t analogChannel(uint8_t pin) {
		if (pin >= A0) pin = static_cast<uint8_t>(pin - A0);
		return pin;
	}

	inline void selectAdcSlot(uint8_t index) {
		ADMUX = adcSlots[index].admux;
		ADCSRB = static_cast<uint8_t>((ADCSRB & ~_BV(MUX5)) | adcSlots[index].mux5);
	}

	inline void startAdcLocked() {
		if (!adcHardwareReady || adcRunning || adcCount == 0) return;
		adcIndex = 0;
		selectAdcSlot(0);
		ADCSRA |= _BV(ADIF); // 1を書いて既存フラグをクリア
		ADCSRA |= _BV(ADSC);
		adcRunning = true;
	}

	inline void adcIsrBody() {
		if (adcCount == 0) {
			adcRunning = false;
			return;
		}

		const uint8_t current = adcIndex;
		*adcSlots[current].dst = ADC;

		uint8_t next = static_cast<uint8_t>(current + 1);
		if (next >= adcCount) next = 0;
		adcIndex = next;

		if (next != current) {
			selectAdcSlot(next);
		}
		ADCSRA |= _BV(ADSC);
	}

} // namespace monocon_detail

volatile uint32_t tms = 0;

di* di::list[8] = {};
uint8_t di::nList = 0;
pr* pr::list[8] = {};
uint8_t pr::nList = 0;
sok* sok::list[2] = {};
uint8_t sok::nList = 0;
enc* enc::list[4] = {};
uint8_t enc::nList = 0;

Led led;
Disp dp;
Dcm dm;
Spm sm;
Bz bz;

bool adcReg(uint8_t pin, volatile int* dst) {
	if (!dst) return false;
	const uint8_t channel = monocon_detail::analogChannel(pin);
	if (channel > 15) return false;

	bool added = false;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		// 同じ保存先の二重登録を防ぐ。
		for (uint8_t i = 0; i < monocon_detail::adcCount; ++i) {
			if (monocon_detail::adcSlots[i].dst == dst) {
				added = true;
				break;
			}
		}

		if (!added && monocon_detail::adcCount < 16) {
			const uint8_t index = monocon_detail::adcCount;
			monocon_detail::adcSlots[index].admux =
				static_cast<uint8_t>(_BV(REFS0) | (channel & 0x07));
			monocon_detail::adcSlots[index].mux5 =
				channel >= 8 ? _BV(MUX5) : 0;
			monocon_detail::adcSlots[index].channel = channel;
			monocon_detail::adcSlots[index].dst = dst;

			// スロットを完全に書き終えてから公開する。
			monocon_detail::adcCount = static_cast<uint8_t>(index + 1);
			if (channel < 8) DIDR0 |= _BV(channel);
			else             DIDR2 |= _BV(channel - 8);
			monocon_detail::startAdcLocked();
			added = true;
		}
	}
	return added;
}

int ar(uint8_t pin) {
	const uint8_t channel = monocon_detail::analogChannel(pin);
	if (channel > 15) return 0;

	volatile int* source = nullptr;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		const uint8_t n = monocon_detail::adcCount;
		for (uint8_t i = 0; i < n; ++i) {
			if (monocon_detail::adcSlots[i].channel == channel) {
				source = monocon_detail::adcSlots[i].dst;
				break;
			}
		}
	}

	if (!source) {
		// 未登録ピンもブロッキングanalogReadをせず、非同期スキャンへ追加する。
		source = &monocon_detail::adcFallback[channel];
		if (!adcReg(pin, source)) return 0;
	}
	return monoconAtomicReadInt(source);
}

ISR(ADC_vect) {
	monocon_detail::adcIsrBody();
}

ISR(TIMER1_COMPA_vect) {
	enc::isrPollAll();
#ifdef useir
	ir();
#endif
}

ISR(TIMER2_COMPA_vect) {
	++tms;
	monocon_detail::serviceDue = 1;
	bz.isrTick();
}

inline void monocon_detail::service() {
	uint8_t due;
	uint32_t now;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		due = monocon_detail::serviceDue;
		monocon_detail::serviceDue = 0;
		now = tms;
	}
	if (!due) return;

	di::serviceAll(now);
	pr::serviceAll(now);
	sok::serviceAll();
	led.serviceTick();
	dp.serviceTick();
}

// loop()終了時に更新し、未読エッジへ1回分のloop()実行機会を与える。
void serialEventRun() {
	monocon_detail::service();
	const uint8_t epoch = ++monocon_detail::loopEpoch;
	di::expireAll(epoch);
	pr::expireAll(epoch);
}

// delay()中は入力更新だけ行い、エッジの寿命は進めない。
// そのためdelay()中に発生したエッジも、次のloop()で1回読み取れる。
void yield() {
	monocon_detail::service();
}

void begin() {
	cli();

	// Mega 2560の固定ピン配置を直接設定。
	DDRF &= static_cast<uint8_t>(~(_BV(PF0) | _BV(PF1) | _BV(PF2) | _BV(PF3)));
	DDRB &= static_cast<uint8_t>(~(_BV(PB4) | _BV(PB5) | _BV(PB6) | _BV(PB7)));

	// 7セグ用固定配線：D6=PH3(SCK), D7=PH4(DATA), D8=PH5(LATCH)。
	DDRH |= SCK_BIT | SDI_BIT | LAT_BIT;
	PORTH = static_cast<uint8_t>(
		(PORTH & static_cast<uint8_t>(~(SCK_BIT | SDI_BIT))) | LAT_BIT);

	DDRE |= _BV(PE3) | _BV(PE4) | _BV(PE5);
	DDRG |= _BV(PG5);
	PORTE &= static_cast<uint8_t>(~(_BV(PE3) | _BV(PE4) | _BV(PE5)));
	PORTG &= static_cast<uint8_t>(~_BV(PG5));

	DDRA |= SPM_MASK;
	PORTA &= static_cast<uint8_t>(~SPM_MASK);

	DDRL |= _BV(PL5) | _BV(PL3);
	PORTL &= static_cast<uint8_t>(~(_BV(PL5) | _BV(PL3)));

	DDRC &= static_cast<uint8_t>(~_BV(PC1)); // D36

	// Timer5: 8-bit phase-correct PWM、/64。Arduino Coreへの暗黙依存を排除。
	TCCR5A = _BV(WGM50);
	TCCR5B = _BV(CS51) | _BV(CS50);
	TCNT5 = 0;
	OCR5A = 0;
	OCR5C = 0;

	// Timer3: ブザー。停止状態のCTC、OC3A切断、ピンLOW。
	TCCR3A = 0;
	TCCR3B = _BV(WGM32);
	TCNT3 = 0;
	OCR3A = 0;
	TIMSK3 = 0;
	TIFR3 = _BV(OCF3A);

	// Timer1: 10 kHz、エンコーダ専用。
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;
	OCR1A = static_cast<uint16_t>((F_CPU / 8UL / 10000UL) - 1UL);
	TCCR1B = _BV(WGM12) | _BV(CS11);
	TIMSK1 = _BV(OCIE1A);
	TIFR1 = _BV(OCF1A);

	// Timer2: 1 kHz。ISRは時刻・実行要求・ブザー時間だけ。
	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2 = 0;
	OCR2A = static_cast<uint8_t>((F_CPU / 64UL / 1000UL) - 1UL);
	TCCR2A = _BV(WGM21);
	TCCR2B = _BV(CS22);
	TIMSK2 = _BV(OCIE2A);
	TIFR2 = _BV(OCF2A);

	// ADC: 単発変換をADC ISRで連鎖。完了待ちポーリングなし。
	ADCSRA = 0;
	ADCSRB &= static_cast<uint8_t>(~(_BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0)));
	ADMUX = _BV(REFS0);
	ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
	monocon_detail::adcHardwareReady = true;
	monocon_detail::adcRunning = false;
	monocon_detail::startAdcLocked();

	tms = 0;
	monocon_detail::serviceDue = 1;
	monocon_detail::loopEpoch = 0;

	sei();

	// 初期出力。delay()は使用しない。
	led(0);
	dp.off();
	dm.fr();
	sm.fr();
	bz.off();
	monocon_detail::service();

	// 暗号用途ではない簡易シード。ADC完了待ちはしない。
	randomSeed(static_cast<unsigned long>(micros()) ^
		static_cast<unsigned long>(TCNT0) ^
		(static_cast<unsigned long>(TCNT1) << 8));
}

#endif // MONOCON_CHUUGOKU_MIN_H
