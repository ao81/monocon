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
		static uint8_t pc = 0;
		bool on = (((ledOpacity + 8) >> 4) > pc);

		dw(LED_R_PIN, (on && (ledColor & 1)) ? HIGH : LOW);
		dw(LED_B_PIN, (on && (ledColor & 2)) ? HIGH : LOW);
		dw(LED_G_PIN, (on && (ledColor & 4)) ? HIGH : LOW);

		pc = (pc + 1) & 15;
	}

	friend void TIMER3_COMPA_vect(void);

public:
	void operator()(uint8_t color, uint8_t opacity = 100) {
		opacity = clamp(opacity, 0, 100);
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
		static uint8_t pc = 0;
		uint8_t a = (((segOpacity[0] + 8) >> 4) > pc) ? segPattern[0] : 0;
		uint8_t b = (((segOpacity[1] + 8) >> 4) > pc) ? segPattern[1] : 0;
		uint8_t c = (((segOpacity[2] + 8) >> 4) > pc) ? segPattern[2] : 0;
		PORTH &= ~LAT_BIT;
		fsout(a);
		fsout(b);
		fsout(c);
		PORTH |= LAT_BIT;
		pc = (pc + 1) & 15;
	}

	friend void TIMER3_COMPA_vect(void);

public:
	void operator()(char a, char b, char c, uint8_t oa = 255, uint8_t ob = 255, uint8_t oc = 255) {
		for (uint8_t i = 0; i < 3; i++) {
			segPattern[i] = (i == 0 ? a : (i == 1 ? b : c));
			segOpacity[i] = (i == 0 ? oa : (i == 1 ? ob : oc));
		}
	}

	void s(const char* s, uint8_t oa = 255, uint8_t ob = 255, uint8_t oc = 255) {
		for (uint8_t i = 0; i < 3; i++) {
			segPattern[i] = toPattern(s[i]);
			segOpacity[i] = (i == 0 ? oa : (i == 1 ? ob : oc));
		}
	}

	void n(int x, bool zero = false, bool left = false) {
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
	}

	void n(double f, bool zero = false, bool left = false) {
		bool neg = f < 0;
		double a = neg ? -f : f;
		if (neg) {
			bool dot = a < 9.95;
			int v = dot ? (int)(a * 10 + 0.5) : (int)(a + 0.5);
			if (v > 99) v = 99;
			uint8_t p1 = seg[(v / 10) % 10];
			if (dot) p1 |= SEG_DOT;
			(*this)(SEG_MINUS, p1, seg[v % 10]);
			return;
		}
		if (zero) {
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
			uint8_t p0 = seg[(v / 100) % 10];
			uint8_t p1 = seg[(v / 10) % 10];
			if (dot == 0) p0 |= SEG_DOT;
			if (dot == 1) p1 |= SEG_DOT;
			(*this)(p0, p1, seg[v % 10]);
			return;
		}
		uint8_t g0, g1;
		if (a < 9.95) {
			int v = (int)(a * 10 + 0.5);
			g0 = seg[(v / 10) % 10] | SEG_DOT;
			g1 = seg[v % 10];
		} else if (a < 99.5) {
			int v = (int)(a + 0.5);
			g0 = seg[(v / 10) % 10];
			g1 = seg[v % 10];
		} else {
			int v = (int)(a + 0.5);
			if (v > 999) v = 999;
			(*this)(seg[(v / 100) % 10], seg[(v / 10) % 10], seg[v % 10]);
			return;
		}
		if (left) (*this)(g0, g1, 0x00);
		else (*this)(0x00, g0, g1);
	}

	void n(float f, bool zero = false, bool left = false) {
		n((double)f, zero, left);
	}

	template <typename T>
	void n(T v, bool zero = false, bool left = false) {
		n((int)v, zero, left);
	}
};
Disp dp;
#define str(...) str(#__VA_ARGS__)

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
