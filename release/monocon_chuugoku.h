/**********************************************/
// 高校生ものづくりコンテスト2026全国大会
//
// 地区名: 中国地区
// 学校名: 岡山県立岡山工業高等学校
// 氏名: 青山 晃大
// 作成年月日: 2026/07/22
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
constexpr uint8_t D_INPUT_MASK = _BV(PB4) | _BV(PB5) | _BV(PB6) | _BV(PB7);

void monoconEarlyDigitalInputInit()
__attribute__((naked, used, section(".init3")));

void monoconEarlyDigitalInputInit() {
	asm volatile(
		"in r24, 0x05\n\t"
		"andi r24, 0x0F\n\t"
		"out 0x05, r24\n\t"
		"in r24, 0x04\n\t"
		"andi r24, 0x0F\n\t"
		"out 0x04, r24\n\t"
		);
}

constexpr uint8_t SCK_BIT = _BV(PH3);
constexpr uint8_t SDI_BIT = _BV(PH4);
constexpr uint8_t LAT_BIT = _BV(PH5);

constexpr uint8_t G = 0b100;
constexpr uint8_t B = 0b010;
constexpr uint8_t R = 0b001;
constexpr uint8_t GB = 0b110;
constexpr uint8_t C = 0b110;
constexpr uint8_t GR = 0b101;
constexpr uint8_t Y = 0b101;
constexpr uint8_t BR = 0b011;
constexpr uint8_t M = 0b011;
constexpr uint8_t GBR = 0b111;

constexpr uint8_t L = LOW;
constexpr uint8_t H = HIGH;

template <typename T, typename U, typename V>

inline T clamp(T v, U lo, V hi) {
	return v < static_cast<T>(lo) ? static_cast<T>(lo)
		: v > static_cast<T>(hi) ? static_cast<T>(hi) : v;
}

template <typename T, typename U, typename V>
inline T wrap(T value, U low, V high) {
	int64_t lo = static_cast<int64_t>(low);
	int64_t hi = static_cast<int64_t>(high);
	if (hi < lo) {
		const int64_t t = lo;
		lo = hi;
		hi = t;
	}
	const int64_t span = hi - lo + 1LL;
	int64_t result = (static_cast<int64_t>(value) - lo) % span;
	if (result < 0) result += span;
	return static_cast<T>(lo + result);
}

extern volatile uint32_t tms;

inline uint32_t atomicMillis() {
	uint32_t v;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { v = tms; }
	return v;
}

inline int atomicReadInt(volatile const int* p) {
	int v;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { v = *p; }
	return v;
}

bool adcReg(uint8_t pin, volatile int* dst);

int ar(uint8_t pin);

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

namespace board_detail {
	extern uint8_t loopEpoch;

	inline void lockDigitalInputs() {
		TCCR0A &= static_cast<uint8_t>(~(_BV(COM0A1) | _BV(COM0A0)));
		TCCR1A &= static_cast<uint8_t>(~(_BV(COM1A1) | _BV(COM1A0) |
			_BV(COM1B1) | _BV(COM1B0)));
		TCCR2A &= static_cast<uint8_t>(~(_BV(COM2A1) | _BV(COM2A0)));
		PORTB &= static_cast<uint8_t>(~D_INPUT_MASK);
		DDRB &= static_cast<uint8_t>(~D_INPUT_MASK);
	}

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
}

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
	bool fHeldRelease;
	uint8_t heldReleaseLevel;
	uint32_t heldReleaseDuration;
	uint8_t heldReleaseEpoch;

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

		if (static_cast<uint32_t>(now - st.candidateSince) < lockMs) {
			return;
		}

		const uint8_t old = st.stable;
		const uint32_t duration =
			static_cast<uint32_t>(st.candidateSince - st.stableSince);

		st.stable = raw;
		st.stableSince = now;
		st.candidateActive = false;
		st.fired = false;

		fHeldRelease = true;
		heldReleaseLevel = old;
		heldReleaseDuration = duration;
		heldReleaseEpoch = board_detail::loopEpoch;

		if (old == LOW && raw == HIGH) {
			fLtoh = true;
			ltohEpoch = board_detail::loopEpoch;
		}

		if (old == HIGH && raw == LOW) {
			fHtol = true;
			htolEpoch = board_detail::loopEpoch;
		}
	}

	void serviceEdges(uint8_t epoch) {
		if (fLtoh && static_cast<uint8_t>(epoch - ltohEpoch) > 1U) {
			fLtoh = false;
		}

		if (fHtol && static_cast<uint8_t>(epoch - htolEpoch) > 1U) {
			fHtol = false;
		}

		if (fHeldRelease &&
			static_cast<uint8_t>(epoch - heldReleaseEpoch) > 1U) {
			fHeldRelease = false;
		}
	}

public:
	explicit InEdge(uint16_t lock = 10)
		: lockMs(lock),
		first(true),
		fLtoh(false),
		fHtol(false),
		ltohEpoch(0),
		htolEpoch(0),
		fHeldRelease(false),
		heldReleaseLevel(LOW),
		heldReleaseDuration(0),
		heldReleaseEpoch(0) {
		st.stable = LOW;
		st.candidate = LOW;
		st.stableSince = 0;
		st.candidateSince = 0;
		st.candidateActive = false;
		st.fired = false;
	}

	bool ltoh() {
		const bool value = fLtoh;
		fLtoh = false;
		return value;
	}

	bool htol() {
		const bool value = fHtol;
		fHtol = false;
		return value;
	}

	bool level() const {
		return st.stable;
	}

	operator bool() const {
		return st.stable;
	}

	bool held(uint16_t ms, bool lv, bool release = false) {
		const uint8_t target = lv ? HIGH : LOW;

		if (release) {
			if (!fHeldRelease || heldReleaseLevel != target) {
				return false;
			}

			fHeldRelease = false;
			return heldReleaseDuration >= ms;
		}

		const uint32_t now = atomicMillis();

		if (st.stable == target &&
			!st.fired &&
			static_cast<uint32_t>(now - st.stableSince) >= ms) {
			st.fired = true;
			return true;
		}

		return false;
	}

	bool change() {
		const bool value = fHtol || fLtoh;
		fHtol = false;
		fLtoh = false;
		return value;
	}
};

class Sig : public InEdge {
private:
	static Sig* head_;
	Sig* next_;

	void attach() {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			next_ = head_;
			head_ = this;
		}
	}

	void detach() {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			Sig** link = &head_;
			while (*link && *link != this) {
				link = &((*link)->next_);
			}
			if (*link == this) {
				*link = next_;
			}
			next_ = nullptr;
		}
	}

public:
	explicit Sig(uint16_t lock = 0)
		: InEdge(lock), next_(nullptr) {
		attach();
	}

	~Sig() {
		detach();
	}

	Sig(const Sig&) = delete;
	Sig& operator=(const Sig&) = delete;

	bool set(bool condition) {
		pollWith(condition ? HIGH : LOW, atomicMillis());
		return level();
	}

	bool update(bool condition) {
		return set(condition);
	}

	bool operator()(bool condition) {
		return set(condition);
	}

	void reset(bool level = false) {
		const uint8_t raw = level ? HIGH : LOW;
		const uint32_t now = atomicMillis();

		first = false;
		st.stable = raw;
		st.candidate = raw;
		st.stableSince = now;
		st.candidateSince = now;
		st.candidateActive = false;
		st.fired = false;
		fLtoh = 0;
		fHtol = 0;
		ltohEpoch = board_detail::loopEpoch;
		htolEpoch = board_detail::loopEpoch;
	}

	bool initialized() const {
		return !first;
	}

	bool changed() {
		const bool result = fLtoh || fHtol;
		fLtoh = 0;
		fHtol = 0;
		return result;
	}

	uint32_t elapsed() const {
		if (first) return 0;
		return static_cast<uint32_t>(atomicMillis() - st.stableSince);
	}

	static void serviceAll(uint8_t epoch) {
		for (Sig* p = head_; p; p = p->next_) {
			p->serviceEdges(epoch);
		}
	}
};

class Di : public InEdge {
private:
	volatile uint8_t* reg;
	uint8_t mask;
	static Di* list[8];
	static uint8_t nList;

public:
	explicit Di(uint8_t pin, uint16_t lock = 10)
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
			Di* const p = list[i];
			p->pollWith((*p->reg & p->mask) ? HIGH : LOW, now);
		}
	}

	static void serviceAll(uint8_t epoch) {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) list[i]->serviceEdges(epoch);
	}
};

class Pr : public InEdge {
private:
	int th;
	volatile int _raw;
	static Pr* list[8];
	static uint8_t nList;

public:
	explicit Pr(uint8_t pin, int threshold = 950, uint16_t lock = 10)
		: InEdge(lock), th(threshold), _raw(0) {
		adcReg(pin, &_raw);
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if (nList < 8) list[nList++] = this;
		}
	}

	int raw() const {
		return atomicReadInt(&_raw);
	}

	static void serviceAll(uint32_t now) {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) {
			Pr* const p = list[i];
			const int value = atomicReadInt(&p->_raw);
			p->pollWith(value > p->th ? HIGH : LOW, now);
		}
	}

	static void serviceAll(uint8_t epoch) {
		const uint8_t n = nList;
		for (uint8_t i = 0; i < n; ++i) list[i]->serviceEdges(epoch);
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

class Sok {
private:
	int ring[5];
	uint8_t ri;
	uint8_t sampleCount;
	bool valid;
	static Sok* list[2];
	static uint8_t nList;

	void serviceOne() {
		const int sample = atomicReadInt(&_raw);
		ring[ri] = sample;
		if (++ri == 5) ri = 0;
		if (sampleCount < 5) ++sampleCount;
		if (sampleCount < 5) return;

		const int med = board_detail::median5(
			ring[0], ring[1], ring[2], ring[3], ring[4]);
		if (valid && med == raw) return;

		raw = med;
		valid = true;
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
		cm = static_cast<float>(m) * 0.1f;
	}

public:
	volatile int _raw;
	int raw;
	int mm;
	float cm;

	explicit Sok(uint8_t pin)
		: ri(0), sampleCount(0), valid(false), _raw(0), raw(0), mm(0), cm(0.0f) {
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

class Vr {
private:
	int inputMin;
	int inputMax;

public:
	volatile int raw;

	explicit Vr(uint8_t pin, int minValue = 0, int maxValue = 512)
		: inputMin(minValue),
		inputMax(maxValue),
		raw(0) {
		adcReg(pin, &raw);
	}

	int to(int lo, int hi) const {
		int v = atomicReadInt(&raw);
		v = clamp(v, inputMin, inputMax);
		const int32_t inSpan = static_cast<int32_t>(inputMax) - inputMin;

		if (inSpan <= 0) return lo;

		const int32_t outSpan = static_cast<int32_t>(hi) - lo;

		int64_t num = static_cast<int64_t>(v - inputMin) * outSpan;

		if (num >= 0) num += inSpan / 2;
		else num -= inSpan / 2;

		return static_cast<int>(
			static_cast<int64_t>(lo) + num / inSpan
			);
	}
};

class Js {
public:
	volatile int x;
	volatile int y;

	Js(uint8_t px, uint8_t py) : x(0), y(0) {
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

		double th = atan2(static_cast<double>(dx), static_cast<double>(dy));
		if (mirror) th = -th;
		th += (rot & 3) * (PI / 2.0);
		int result = static_cast<int>((th + 4.0 * PI + PI / div) /
			(2.0 * PI / div)) % div;
		if (result < 0) result += div;
		return result;
	}
};

extern "C" {
	void PCINT0_vect(void);
	void PCINT1_vect(void);
	void PCINT2_vect(void);
	void TIMER1_COMPA_vect(void);
}

class Enc {
	friend void begin();
	friend void ::PCINT0_vect(void);
	friend void ::PCINT1_vect(void);
	friend void ::PCINT2_vect(void);
	friend void ::TIMER1_COMPA_vect(void);

private:
	uint8_t pinA;
	uint8_t pinB;
	volatile uint8_t* ra;
	volatile uint8_t* rb;
	uint8_t ma;
	uint8_t mb;
	bool samePort;
	volatile uint8_t est;
	volatile int32_t pending;
	bool direction;

	static Enc* list[4];
	static uint8_t nList;
	static Enc* fallback[4];
	static uint8_t nFallback;
	static Enc* pcOwner[3][8];
	static volatile uint8_t* pcPort[3];
	static uint8_t pcWatched[3];
	static uint8_t pcPrevious[3];

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

		const uint8_t next =
			table[est & 0x0F][(a << 1) | b];

		est = next;

		const uint8_t event = next & 0x30;

		if (event == 0x10) {
			if (direction) {
				if (pending < INT32_MAX) ++pending;
			} else {
				if (pending > INT32_MIN) --pending;
			}
		} else if (event == 0x20) {
			if (direction) {
				if (pending > INT32_MIN) --pending;
			} else {
				if (pending < INT32_MAX) ++pending;
			}
		}
	}

	int32_t take() {
		int32_t v;

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			v = pending;
			pending = 0;
		}

		return v;
	}

public:
	Enc(uint8_t pa, uint8_t pb, bool d = true)
		: pinA(pa),
		pinB(pb),
		ra(portInputRegister(digitalPinToPort(pa))),
		rb(portInputRegister(digitalPinToPort(pb))),
		ma(digitalPinToBitMask(pa)),
		mb(digitalPinToBitMask(pb)),
		samePort(ra == rb),
		est(0),
		pending(0),
		direction(d) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if (nList < 4) {
				list[nList++] = this;
			}
		}
	}

private:
	static void beginPolling(bool forceTimer = false) {
		nFallback = 0;
		for (uint8_t g = 0; g < 3; ++g) {
			pcPort[g] = nullptr;
			pcWatched[g] = 0;
			pcPrevious[g] = 0;
			for (uint8_t b = 0; b < 8; ++b) {
				pcOwner[g][b] = nullptr;
			}
		}

		uint8_t enabledGroups = 0;

		for (uint8_t i = 0; i < nList; ++i) {
			Enc* const p = list[i];
			bool attached = false;

#if defined(digitalPinToPCICR) && defined(digitalPinToPCICRbit) && \
	defined(digitalPinToPCMSK) && defined(digitalPinToPCMSKbit)
			volatile uint8_t* const pcrA = digitalPinToPCICR(p->pinA);
			volatile uint8_t* const pcrB = digitalPinToPCICR(p->pinB);
			volatile uint8_t* const pmskA = digitalPinToPCMSK(p->pinA);
			volatile uint8_t* const pmskB = digitalPinToPCMSK(p->pinB);

			if (pcrA && pcrB && pmskA && pmskB) {
				const uint8_t groupA = digitalPinToPCICRbit(p->pinA);
				const uint8_t groupB = digitalPinToPCICRbit(p->pinB);
				const uint8_t bitA = digitalPinToPCMSKbit(p->pinA);
				const uint8_t bitB = digitalPinToPCMSKbit(p->pinB);

				if (groupA < 3 && groupB < 3 && bitA < 8 && bitB < 8 &&
					!(groupA == groupB && bitA == bitB) &&
					(!pcPort[groupA] || pcPort[groupA] == p->ra) &&
					(!pcPort[groupB] || pcPort[groupB] == p->rb) &&
					!pcOwner[groupA][bitA] && !pcOwner[groupB][bitB]) {
					pcPort[groupA] = p->ra;
					pcPort[groupB] = p->rb;
					pcOwner[groupA][bitA] = p;
					pcOwner[groupB][bitB] = p;
					pcWatched[groupA] |= _BV(bitA);
					pcWatched[groupB] |= _BV(bitB);
					*pmskA |= _BV(bitA);
					*pmskB |= _BV(bitB);
					*pcrA |= _BV(groupA);
					*pcrB |= _BV(groupB);
					enabledGroups |= _BV(groupA) | _BV(groupB);
					attached = true;
				}
			}
#endif

			if (!attached && nFallback < 4) {
				fallback[nFallback++] = p;
			}
		}

		for (uint8_t g = 0; g < 3; ++g) {
			if (pcPort[g]) {
				pcPrevious[g] = *pcPort[g];
			}
		}

		if (enabledGroups) {
			PCIFR = enabledGroups;
		}

		TCCR1A = 0;
		TCCR1B = 0;
		TCNT1 = 0;
		OCR1A = static_cast<uint16_t>((F_CPU / 8UL / 10000UL) - 1UL);
		TIFR1 = _BV(OCF1A);
		TIMSK1 = (nFallback || forceTimer) ? _BV(OCIE1A) : 0;
		if (nFallback || forceTimer) {
			TCCR1B = _BV(WGM12) | _BV(CS11);
		}
	}

	static inline void isrPollFallback() {
		const uint8_t n = nFallback;
		if (n > 0) fallback[0]->poll();
		if (n > 1) fallback[1]->poll();
		if (n > 2) fallback[2]->poll();
		if (n > 3) fallback[3]->poll();
	}

	static inline void isrPcint(uint8_t group) {
		volatile uint8_t* const port = pcPort[group];
		if (!port) return;

		const uint8_t now = *port;
		uint8_t changed =
			static_cast<uint8_t>((now ^ pcPrevious[group]) & pcWatched[group]);
		pcPrevious[group] = now;

		while (changed) {
			uint8_t bit = 0;
			while (!(changed & _BV(bit))) ++bit;
			changed &= static_cast<uint8_t>(~_BV(bit));
			Enc* const p = pcOwner[group][bit];
			if (!p) continue;
			for (uint8_t next = static_cast<uint8_t>(bit + 1); next < 8; ++next) {
				if (pcOwner[group][next] == p) {
					changed &= static_cast<uint8_t>(~_BV(next));
				}
			}
			p->poll();
		}
	}

public:
	int32_t delta() {
		return take();
	}

	int32_t clampTo(
		int32_t value,
		int32_t lo,
		int32_t hi
	) {
		if (hi < lo) {
			const int32_t t = lo;
			lo = hi;
			hi = t;
		}

		int64_t result =
			static_cast<int64_t>(value) + take();

		if (result < lo) result = lo;
		if (result > hi) result = hi;

		return static_cast<int32_t>(result);
	}

	int32_t loopTo(
		int32_t value,
		int32_t lo,
		int32_t hi
	) {
		if (hi < lo) {
			const int32_t t = lo;
			lo = hi;
			hi = t;
		}

		const int64_t span =
			static_cast<int64_t>(hi) - lo + 1LL;

		int64_t result =
			static_cast<int64_t>(value) +
			take() -
			lo;

		result %= span;

		if (result < 0) {
			result += span;
		}

		return static_cast<int32_t>(
			static_cast<int64_t>(lo) + result
			);
	}
};

class Led {
private:
	uint8_t color;
	uint8_t opacity;
	uint8_t acc;
	uint8_t previousState;

	void writeState(uint8_t state) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			uint8_t pe = PORTE & static_cast<uint8_t>(~(_BV(PE4) | _BV(PE5)));
			if (state & B) pe |= _BV(PE5);
			if (state & G) pe |= _BV(PE4);
			PORTE = pe;

			if (state & R) PORTG |= _BV(PG5);
			else           PORTG &= static_cast<uint8_t>(~_BV(PG5));
		}
	}

public:
	Led() : color(0), opacity(255), acc(0), previousState(0xFF) {}

	void operator()(uint8_t newColor, uint8_t opacityPercent = 100) {
		newColor &= 0x07;
		const uint8_t newOpacity = board_detail::percentToByte(
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
			state = color;
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

	static uint8_t digitPattern(uint8_t value) {
		if (value < 10) return seg[value];
		if (value < 36) return alp[value - 10];
		return SEG_NONE;
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
		if (!text || !text[0]) return off();
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
		if (!isfinite(f)) return s("Err");
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
			opacity[i] = board_detail::percentToByte(p);
		}
		return *this;
	}

	Disp& base(int32_t x, uint8_t radix, bool zero = false) {
		if (radix < 2 || radix > 36) {
			return s("Err");
		}

		const uint32_t range =
			static_cast<uint32_t>(radix) *
			static_cast<uint32_t>(radix) *
			static_cast<uint32_t>(radix);

		int64_t value = static_cast<int64_t>(x) % range;

		if (value < 0) {
			value += range;
		}

		const uint8_t d2 = value % radix;
		value /= radix;

		const uint8_t d1 = value % radix;
		value /= radix;

		const uint8_t d0 = value % radix;

		const uint8_t p0 = digitPattern(d0);
		const uint8_t p1 = digitPattern(d1);
		const uint8_t p2 = digitPattern(d2);

		if (zero) {
			return (*this)(p0, p1, p2);
		}

		if (d0 != 0) {
			return (*this)(p0, p1, p2);
		}

		if (d1 != 0) {
			return (*this)(0, p1, p2);
		}

		return (*this)(0, 0, p2);
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
			board_detail::writeDisplay3(out[0], out[1], out[2]);
			previous[0] = out[0];
			previous[1] = out[1];
			previous[2] = out[2];
		}
	}
};

class Dcm {
private:
	volatile uint32_t remainingMs;
	volatile bool timedActive;
	volatile bool donePending;

	inline void stopFromIsr() {
		TCCR5A &= static_cast<uint8_t>(~(_BV(COM5A1) | _BV(COM5C1)));
		PORTL &= static_cast<uint8_t>(~(_BV(PL5) | _BV(PL3)));
		now = 0;
		timedActive = false;
		remainingMs = 0;
	}

public:
	volatile int8_t now;

	Dcm()
		: remainingMs(0), timedActive(false), donePending(false), now(0) {
	}

	void cw(int spd) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			timedActive = false;
			remainingMs = 0;
			donePending = false;
		}
		const uint8_t pwm = static_cast<uint8_t>(clamp<int, int, int>(spd, 0, 255));
		if (pwm == 0) { fr(); return; }
		TCCR5A &= static_cast<uint8_t>(~(_BV(COM5A1) | _BV(COM5C1)));
		PORTL &= static_cast<uint8_t>(~(_BV(PL5) | _BV(PL3)));
		OCR5A = pwm;
		TCCR5A |= _BV(COM5A1);
		now = 1;
	}

	void cw(int spd, uint32_t durationMs) {
		cw(spd);
		if (now == 0 || durationMs == 0) return;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			remainingMs = durationMs;
			timedActive = true;
			donePending = false;
		}
	}

	void ccw(int spd) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			timedActive = false;
			remainingMs = 0;
			donePending = false;
		}
		const uint8_t pwm = static_cast<uint8_t>(clamp<int, int, int>(spd, 0, 255));
		if (pwm == 0) { fr(); return; }
		TCCR5A &= static_cast<uint8_t>(~(_BV(COM5A1) | _BV(COM5C1)));
		PORTL &= static_cast<uint8_t>(~(_BV(PL5) | _BV(PL3)));
		OCR5C = pwm;
		TCCR5A |= _BV(COM5C1);
		now = -1;
	}

	void ccw(int spd, uint32_t durationMs) {
		ccw(spd);
		if (now == 0 || durationMs == 0) return;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			remainingMs = durationMs;
			timedActive = true;
			donePending = false;
		}
	}

	void br() {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			timedActive = false;
			remainingMs = 0;
			donePending = false;
			TCCR5A &= static_cast<uint8_t>(~(_BV(COM5A1) | _BV(COM5C1)));
			PORTL |= _BV(PL5) | _BV(PL3);
			now = 0;
		}
	}

	void fr() {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			stopFromIsr();
			donePending = false;
		}
	}

	bool busy() const {
		return timedActive;
	}

	bool done() {
		bool value;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			value = donePending;
			donePending = false;
		}
		return value;
	}

	inline void isrTick() {
		if (!timedActive) return;
		if (remainingMs > 0 && --remainingMs == 0) {
			stopFromIsr();
			donePending = true;
		}
	}
};

constexpr uint8_t spmPhaseMask1[4] = {
	_BV(PA0),
	_BV(PA6),
	_BV(PA4),
	_BV(PA2)
};

constexpr uint8_t spmPhaseMask2[4] = {
	static_cast<uint8_t>(_BV(PA6) | _BV(PA0)),
	static_cast<uint8_t>(_BV(PA6) | _BV(PA4)),
	static_cast<uint8_t>(_BV(PA4) | _BV(PA2)),
	static_cast<uint8_t>(_BV(PA2) | _BV(PA0))
};

constexpr uint8_t SPM_MASK =
_BV(PA6) | _BV(PA4) | _BV(PA2) | _BV(PA0);

enum Dir : uint8_t {
	CW,
	CCW,
	SHORT
};

class Spm {
private:
	enum Excitation : uint8_t {
		ONE_PHASE,
		TWO_PHASE
	};

	static constexpr int32_t STEPS = 2048;

	uint8_t ix;
	Excitation excitation;

	int32_t currentStep;
	int32_t targetStep;

	uint32_t previousStepMs;

	void phase(uint8_t s) {
		ix = s & 3;

		const uint8_t mask =
			excitation == ONE_PHASE
			? spmPhaseMask1[ix]
			: spmPhaseMask2[ix];

		PORTA = static_cast<uint8_t>(
			(PORTA & static_cast<uint8_t>(~SPM_MASK)) | mask
			);
	}

	void mode(Excitation newMode) {
		excitation = newMode;
		phase(ix);
	}

	int32_t degreeToStep(float degree) const {
		const float value = degree * static_cast<float>(STEPS) / 360.0f;

		return value >= 0.0f
			? static_cast<int32_t>(value + 0.5f)
			: static_cast<int32_t>(value - 0.5f);
	}

	void stepCw() {
		phase((ix + 3) & 3);
		--currentStep;
	}

	void stepCcw() {
		phase((ix + 1) & 3);
		++currentStep;
	}

public:
	explicit Spm(Excitation mode = TWO_PHASE)
		: ix(0),
		excitation(mode),
		currentStep(0),
		targetStep(0),
		previousStepMs(0) {
	}

	void cw() {
		targetStep = currentStep;
		stepCcw();
		targetStep = currentStep;
	}

	void ccw() {
		targetStep = currentStep;
		stepCw();
		targetStep = currentStep;
	}

	void fr() {
		targetStep = currentStep;
		PORTA &= static_cast<uint8_t>(~SPM_MASK);
	}

	void br() {
		targetStep = currentStep;
		phase(ix);
	}

	void _one() {
		mode(ONE_PHASE);
	}

	void _two() {
		mode(TWO_PHASE);
	}

	void rela(float degree) {
		targetStep += degreeToStep(degree);
	}

	void abso(float degree, Dir dir = SHORT, Dir halfDir = CCW) {
		degree = fmod(degree, 360.0f);

		if (degree < 0.0f) {
			degree += 360.0f;
		}

		const int32_t destination = degreeToStep(degree);

		int32_t current = currentStep % STEPS;

		if (current < 0) {
			current += STEPS;
		}

		int32_t diff = destination - current;
		const int32_t half = STEPS / 2;

		switch (dir) {
		case CW:
			if (diff < 0) {
				diff += STEPS;
			}
			break;

		case CCW:
			if (diff > 0) {
				diff -= STEPS;
			}
			break;

		case SHORT:
			if (diff > half) {
				diff -= STEPS;
			} else if (diff < -half) {
				diff += STEPS;
			} else if (diff == half || diff == -half) {
				diff = halfDir == CW ? -half : half;
			}
			break;
		}

		targetStep = currentStep + diff;
	}

	void update(uint32_t intervalMs) {
		if (currentStep == targetStep) {
			return;
		}

		if (intervalMs == 0) {
			intervalMs = 1;
		}

		const uint32_t now = millis();

		if (static_cast<uint32_t>(now - previousStepMs) < intervalMs) {
			return;
		}

		previousStepMs = now;

		if (currentStep < targetStep) {
			stepCcw();
		} else {
			stepCw();
		}
	}

	bool busy() const {
		return currentStep != targetStep;
	}

	void stop() {
		targetStep = currentStep;
	}

	void zero() {
		currentStep = 0;
		targetStep = 0;
	}

	float pos() const {
		return static_cast<float>(currentStep) * 360.0f /
			static_cast<float>(STEPS);
	}

	int32_t stepPos() const {
		return currentStep;
	}
};

class Bz {
private:
	int continuousFrequency;
	volatile uint32_t remainingMs;
	volatile bool timedActive;
	const int* melodyNotes;
	const int* melodyDurations;
	int melodyLength;
	int melodyIndex;
	uint32_t melodyNext;
	bool melodyRunning;
	bool melodyRepeat;

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

		const uint16_t top = topForFrequency(f);

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			TCCR3B = _BV(WGM32);
			TCCR3A &= static_cast<uint8_t>(~_BV(COM3A0));
			PORTE &= static_cast<uint8_t>(~_BV(PE3));

			TCNT3 = 0;
			OCR3A = top;
			TIFR3 = _BV(OCF3A);

			remainingMs = durationMs;
			timedActive = timed && durationMs != 0;

			TCCR3A |= _BV(COM3A0);
			TCCR3B = _BV(WGM32) | _BV(CS31);
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
	Bz()
		: continuousFrequency(-1), remainingMs(0), timedActive(false),
		melodyNotes(nullptr), melodyDurations(nullptr), melodyLength(0),
		melodyIndex(0), melodyNext(0), melodyRunning(false),
		melodyRepeat(false) {
	}

	void operator()(int frequency) {
		melodyRunning = false;

		if (frequency <= 0) {
			off();
			return;
		}

		if (timedActive) {
			continuousFrequency = frequency;
			start(frequency, 0, false);
			return;
		}

		if (continuousFrequency == frequency) {
			return;
		}

		if (continuousFrequency <= 0) {
			continuousFrequency = frequency;
			start(frequency, 0, false);
			return;
		}

		const uint16_t newTop = topForFrequency(frequency);

		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			OCR3A = newTop;

			if (TCNT3 > newTop) {
				TCNT3 = 0;
			}
		}

		continuousFrequency = frequency;
	}

	void operator()(int f, uint32_t durationMs) {
		if (durationMs == 0) {
			(*this)(f);
			return;
		}
		melodyRunning = false;
		continuousFrequency = -1;
		start(f, durationMs, true);
	}

	void play(const int* notes, const int* durations, int length,
		bool repeat = false) {
		if (!notes || !durations || length <= 0) {
			stop();
			return;
		}
		stop();
		melodyNotes = notes;
		melodyDurations = durations;
		melodyLength = length;
		melodyIndex = 0;
		melodyNext = atomicMillis();
		melodyRepeat = repeat;
		melodyRunning = true;
		update();
	}

	void stop() {
		melodyRunning = false;
		continuousFrequency = -1;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			stopFromIsr();
		}
	}

	void off() { stop(); }

	bool playing() const { return melodyRunning; }

	void update() {
		if (!melodyRunning) return;
		const uint32_t now = atomicMillis();
		if (static_cast<int32_t>(now - melodyNext) < 0) return;

		if (melodyIndex >= melodyLength) {
			if (melodyRepeat) melodyIndex = 0;
			else {
				stop();
				return;
			}
		}

		const int frequency = melodyNotes[melodyIndex];
		const int duration = melodyDurations[melodyIndex];
		++melodyIndex;
		melodyNext = now + static_cast<uint32_t>(duration > 0 ? duration : 0);
		continuousFrequency = -1;

		if (frequency > 0) {
			start(frequency, 0, false);
		} else {
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
				stopFromIsr();
			}
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

class Seq {
private:
	int current_ = 0;
	int position_ = 0;
	int count_ = 0;
	int target_ = 0;

	uint32_t enteredAt_ = 0;
	uint8_t lastEpoch_ = 0xFF;

	bool initialized_ = false;
	bool transitionPending_ = false;
	bool transitionGrace_ = false;
	bool restartPending_ = false;
	bool entryPending_ = true;
	bool exitPending_ = false;
	bool exitConsumed_ = false;

	int clampState(int state) const {
		if (state < 0) {
			return 0;
		}

		if (count_ > 0 && state >= count_) {
			return count_ - 1;
		}

		return state;
	}

	void syncLoop() {
		const uint8_t epoch = board_detail::loopEpoch;

		if (lastEpoch_ == epoch) {
			return;
		}

		if (position_ > count_) {
			count_ = position_;
		}

		if (transitionPending_) {
			if (exitConsumed_ || !transitionGrace_) {
				current_ = clampState(target_);
				enteredAt_ = millis();
				entryPending_ = true;
				transitionPending_ = false;
				transitionGrace_ = false;
				exitPending_ = false;
				exitConsumed_ = false;
			} else {
				transitionGrace_ = false;
				exitPending_ = true;
				exitConsumed_ = false;
			}
		} else if (restartPending_) {
			enteredAt_ = millis();
			entryPending_ = true;
			restartPending_ = false;
		}

		position_ = 0;
		lastEpoch_ = epoch;

		if (!initialized_) {
			enteredAt_ = millis();
			initialized_ = true;
		}
	}

	void moveTo(int state) {
		syncLoop();

		state = clampState(state);

		if (!transitionPending_ && state == current_) {
			return;
		}

		if (transitionPending_ && state == target_) {
			return;
		}

		const bool currentPassed = position_ > current_;

		if (!transitionPending_) {
			transitionGrace_ = currentPassed;
			exitConsumed_ = false;
		} else if (currentPassed && !exitConsumed_) {
			transitionGrace_ = true;
		}

		target_ = state;
		transitionPending_ = true;
		restartPending_ = false;

		if (!exitConsumed_) {
			exitPending_ = true;
		}
	}

public:
	Seq() = default;

	Seq(const Seq&) = delete;

	Seq& operator=(const Seq&) = delete;

	bool on() {
		syncLoop();
		return position_++ == current_;
	}

	bool operator()() {
		return on();
	}

	explicit operator bool() {
		return on();
	}

	void next() {
		syncLoop();

		if (transitionPending_) {
			return;
		}

		int target = current_ + 1;

		if (count_ > 0 && target >= count_) {
			target = 0;
		}

		moveTo(target);
	}

	void prev() {
		syncLoop();

		if (transitionPending_) {
			return;
		}

		int target = current_ - 1;

		if (target < 0) {
			target = count_ > 0 ? count_ - 1 : 0;
		}

		moveTo(target);
	}

	void to(int state) {
		moveTo(state);
	}

	void restart() {
		syncLoop();

		transitionPending_ = false;
		transitionGrace_ = false;
		restartPending_ = true;
		exitPending_ = false;
		exitConsumed_ = false;
	}

	bool is(int state) {
		syncLoop();
		return current_ == state;
	}

	int now() {
		syncLoop();
		return current_;
	}

	int steps() {
		syncLoop();
		return count_;
	}

	bool in() {
		syncLoop();

		if (!entryPending_) {
			return false;
		}

		entryPending_ = false;
		return true;
	}

	bool out() {
		syncLoop();

		if (!exitPending_) {
			return false;
		}

		exitPending_ = false;
		exitConsumed_ = true;
		return true;
	}

	uint32_t elapsed() {
		syncLoop();

		return static_cast<uint32_t>(
			millis() - enteredAt_
			);
	}

	bool after(uint32_t ms) {
		return elapsed() >= ms;
	}
};

class Iv {
private:
	uint32_t previous_ = 0;
	uint32_t pausedAt_ = 0;
	bool paused_ = false;
	bool immediate_ = false;

public:
	bool operator()(uint32_t ms) {
		if (paused_) return false;

		const uint32_t now = millis();

		if (immediate_) {
			immediate_ = false;
			previous_ = now;
			return true;
		}

		if (static_cast<uint32_t>(now - previous_) < ms) {
			return false;
		}

		previous_ = now;
		return true;
	}

	void reset(bool immediate = false) {
		previous_ = millis();
		immediate_ = immediate;
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

class Tog {
	bool value_;

public:
	explicit Tog(bool initial = true)
		: value_(initial) {
	}

	bool operator()() {
		const bool value = value_;
		value_ = !value_;
		return value;
	}

	template<class T>
	T operator()(T first, T second) {
		return (*this)() ? first : second;
	}

	operator bool() const {
		return value_;
	}

	bool get() const {
		return value_;
	}

	void flip() {
		value_ = !value_;
	}

	void reset(bool initial = true) {
		value_ = initial;
	}
};

namespace board_detail {
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
	volatile bool servicePending = false;
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
		ADCSRA |= _BV(ADIF);
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
}

volatile uint32_t tms = 0;

Sig* Sig::head_ = nullptr;
Di* Di::list[8] = {};
uint8_t Di::nList = 0;
Pr* Pr::list[8] = {};
uint8_t Pr::nList = 0;
Sok* Sok::list[2] = {};
uint8_t Sok::nList = 0;
Enc* Enc::list[4] = {};
uint8_t Enc::nList = 0;
Enc* Enc::fallback[4] = {};
uint8_t Enc::nFallback = 0;
Enc* Enc::pcOwner[3][8] = {};
volatile uint8_t* Enc::pcPort[3] = {};
uint8_t Enc::pcWatched[3] = {};
uint8_t Enc::pcPrevious[3] = {};

Led led;
Disp dp;
Dcm dm;
Spm sm;
Bz bz;

bool adcReg(uint8_t pin, volatile int* dst) {
	if (!dst) return false;
	const uint8_t channel = board_detail::analogChannel(pin);
	if (channel > 15) return false;

	bool added = false;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		for (uint8_t i = 0; i < board_detail::adcCount; ++i) {
			if (board_detail::adcSlots[i].dst == dst) {
				added = true;
				break;
			}
		}

		if (!added && board_detail::adcCount < 16) {
			const uint8_t index = board_detail::adcCount;
			board_detail::adcSlots[index].admux =
				static_cast<uint8_t>(_BV(REFS0) | (channel & 0x07));
			board_detail::adcSlots[index].mux5 =
				channel >= 8 ? _BV(MUX5) : 0;
			board_detail::adcSlots[index].channel = channel;
			board_detail::adcSlots[index].dst = dst;

			board_detail::adcCount = static_cast<uint8_t>(index + 1);
			if (channel < 8) DIDR0 |= _BV(channel);
			else             DIDR2 |= _BV(channel - 8);
			board_detail::startAdcLocked();
			added = true;
		}
	}
	return added;
}

int ar(uint8_t pin) {
	const uint8_t channel = board_detail::analogChannel(pin);
	if (channel > 15) return 0;

	volatile int* source = nullptr;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		const uint8_t n = board_detail::adcCount;
		for (uint8_t i = 0; i < n; ++i) {
			if (board_detail::adcSlots[i].channel == channel) {
				source = board_detail::adcSlots[i].dst;
				break;
			}
		}
	}

	if (!source) {
		source = &board_detail::adcFallback[channel];
		if (!adcReg(pin, source)) return 0;
	}
	return atomicReadInt(source);
}

ISR(ADC_vect) {
	board_detail::adcIsrBody();
}

ISR(PCINT0_vect) {
	Enc::isrPcint(0);
}

ISR(PCINT1_vect) {
	Enc::isrPcint(1);
}

ISR(PCINT2_vect) {
	Enc::isrPcint(2);
}

ISR(TIMER1_COMPA_vect) {
	Enc::isrPollFallback();
#ifdef useir
	ir();
#endif
}

ISR(TIMER2_COMPA_vect) {
	++tms;
	board_detail::servicePending = true;
	dm.isrTick();
	bz.isrTick();
}

inline void board_detail::service() {
	lockDigitalInputs();
	bool pending;
	uint32_t now;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		pending = board_detail::servicePending;
		board_detail::servicePending = false;
		now = tms;
	}
	if (!pending) return;

	Di::serviceAll(now);
	Pr::serviceAll(now);
	Sok::serviceAll();
	bz.update();
	led.serviceTick();
	dp.serviceTick();
}

void yield() {
	board_detail::service();
}

void begin() {
	cli();

	DDRF &= static_cast<uint8_t>(~(_BV(PF0) | _BV(PF1) | _BV(PF2) | _BV(PF3)));
	board_detail::lockDigitalInputs();

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

	DDRC &= static_cast<uint8_t>(~_BV(PC1));

	TCCR5A = _BV(WGM50);
	TCCR5B = _BV(CS51) | _BV(CS50);
	TCNT5 = 0;
	OCR5A = 0;
	OCR5C = 0;

	TCCR3A = 0;
	TCCR3B = _BV(WGM32);
	TCNT3 = 0;
	OCR3A = 0;
	TIMSK3 = 0;
	TIFR3 = _BV(OCF3A);

#ifdef useir
	Enc::beginPolling(true);
#else
	Enc::beginPolling(false);
#endif

	TCCR2A = 0;
	TCCR2B = 0;
	TCNT2 = 0;
	OCR2A = static_cast<uint8_t>((F_CPU / 64UL / 1000UL) - 1UL);
	TCCR2A = _BV(WGM21);
	TCCR2B = _BV(CS22);
	TIMSK2 = _BV(OCIE2A);
	TIFR2 = _BV(OCF2A);

	ADCSRA = 0;
	ADCSRB &= static_cast<uint8_t>(~(_BV(ADTS2) | _BV(ADTS1) | _BV(ADTS0)));
	ADMUX = _BV(REFS0);
	ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
	board_detail::adcHardwareReady = true;
	board_detail::adcRunning = false;
	board_detail::startAdcLocked();

	tms = 0;
	board_detail::servicePending = true;
	board_detail::loopEpoch = 0;

	sei();

	led(0);
	dp.off();
	dm.fr();
	sm.fr();
	bz.off();
	board_detail::service();

	randomSeed(static_cast<unsigned long>(micros()) ^
		static_cast<unsigned long>(TCNT0) ^
		(static_cast<unsigned long>(TCNT1) << 8));
}

void userSetup() __attribute__((weak));
void userLoop() __attribute__((weak));

void setup() {
#ifdef dbg
	Serial.begin(115200);
#endif

	begin();

	if (userSetup) {
		userSetup();
	}
}

void loop() {
	if (userLoop) {
		userLoop();
	}

	board_detail::service();

	const uint8_t epoch = ++board_detail::loopEpoch;
	Sig::serviceAll(epoch);
	Di::serviceAll(epoch);
	Pr::serviceAll(epoch);
}

#define setup userSetup
#define loop userLoop

#ifdef dbg

inline void debugLine() {
	Serial.println();
}

template <typename T, typename... Rest>
inline void debugLine(const T& value, const Rest&... rest) {
	Serial.print(value);
	debugLine(rest...);
}

#define D(...) do { debugLine(__VA_ARGS__); } while (0)

#define DP(x) do { \
	Serial.print(x); \
} while (0)

#define DV(x) do { \
	Serial.print(F(#x " = ")); \
	Serial.println(x); \
} while (0)

#define DT(x) do { \
	Serial.print('['); \
	Serial.print(millis()); \
	Serial.print(F("] ")); \
	Serial.println(F(x)); \
} while (0)

#define DC(x) do { \
	auto _dcNow = (x); \
	static decltype(_dcNow) _dcPrevious = _dcNow; \
	if (_dcNow != _dcPrevious) { \
		Serial.print(F(#x ": ")); \
		Serial.print(_dcPrevious); \
		Serial.print(F(" -> ")); \
		Serial.println(_dcNow); \
		_dcPrevious = _dcNow; \
	} \
} while (0)

#define DH() do { \
	Serial.print(__FILE__); \
	Serial.print(':'); \
	Serial.println(__LINE__); \
} while (0)

#else

#define D(...) do {} while (0)
#define DP(x) do {} while (0)
#define DV(x) do {} while (0)
#define DT(x) do {} while (0)
#define DC(x) do {} while (0)
#define DH() do {} while (0)

#endif

#endif
