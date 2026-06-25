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

#define p1 A1
#define p2 A2
#define p3 17
#define p4 18
#define p5 19
#define p6 40
#define p7 42
#define p8 20

#define LAT1_PIN 22
#define LAT2_PIN 23
#define SCK_PIN  24
#define SDI_PIN  25

#define SDI_BIT  (1 << PA3)
#define SCK_BIT  (1 << PA2)
#define LAT1_BIT (1 << PA0)
#define LAT2_BIT (1 << PA1)

inline void fsout(uint8_t v) {
	for (uint8_t i = 0; i < 8; i++) {
		if (v & 0x80) PORTA |=  SDI_BIT;
		else PORTA &= ~SDI_BIT;
		PORTA |= SCK_BIT;
		PORTA &= ~SCK_BIT;
		v <<= 1;
	}
}

inline int ar(uint8_t pin) {
	if (pin >= A0) pin -= A0;
	ADMUX  = (1 << REFS0) | (pin & 0x07);
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

	/* 測距モジュール (ranging module) */
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
			if (d) c++;
			else c--;
		} else if (dir == 0x20) {
			if (d) c--;
			else c++;
		}

		pa = a;
		pb = b;
	}

	/* フォトリフレクタ */
	int p = 0;

	void fpr(int pin) {
		p = ar(pin);
	}
};
In in;

int& sw = in.sw;
int& ts = in.ts;
int& ph = in.ph;
int& rm = in.rm;
int& c = in.c;
int& p = in.p;

void disp(char a, char b) {
	PORTA &= ~LAT1_BIT;
	fsout(a);
	fsout(b);
	PORTA |= LAT1_BIT;
}

void isr();
ISR(TIMER3_COMPA_vect) {
	isr();
}

void begin(void) {
	pinMode(p1, INPUT);
	pinMode(p2, INPUT);
	pinMode(p3, INPUT);
	pinMode(p4, INPUT);
	pinMode(p5, INPUT);
	pinMode(p6, INPUT);
	pinMode(p7, INPUT);
	pinMode(p8, OUTPUT);
	digitalWrite(p8, LOW);

	pinMode(LAT1_PIN, OUTPUT);
	pinMode(SCK_PIN, OUTPUT);
	pinMode(SDI_PIN, OUTPUT);

	digitalWrite(LAT1_PIN, HIGH);
	digitalWrite(SCK_PIN, LOW);
	digitalWrite(SDI_PIN, LOW);

	ADCSRA = (ADCSRA & ~0x07) | (1 << ADPS2);

	cli();
	TCCR3A = TCCR3B = TCNT3 = 0;
	OCR3A = F_CPU / 64 / 10000 - 1; // 100μs
	TCCR3B |= (1 << WGM32);
	TCCR3B |= (1 << CS31) | (1 << CS30);
	TIMSK3 |= (1 << OCIE3A);
	sei();
}
