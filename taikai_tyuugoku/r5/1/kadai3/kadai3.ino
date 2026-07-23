// 43:05 あきらめ

#define USE_TIMER3_ISR
#include "mono_con.h"
#define lmc lm.color.GBR

// A, B, C, D, E, F, O, Q
const int alpha[8] = { 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x5c, 0xbf };
const int minas = 0x40;

const int c[4] = { 1, 0, 1, 0 };
const int e[1] = { 0 };
const int h[4] = { 0, 0, 0, 0 };
const int l[4] = { 0, 1, 0, 0 };
const int o[3] = { 1, 1, 1 };
const int q[4] = { 1, 1, 0, 1 };
const int _1[5] = { 0, 1, 1, 1, 1 };
const int _2[5] = { 0, 0, 1, 1, 1 };
const int _3[5] = { 0, 0, 0, 1, 1 };
const int _4[5] = { 0, 0, 0, 0, 1 };

const int* morse[] = { c, e, h, l, o, q, _1, _2, _3, _4 };
const int size[] = { 4, 1, 4, 4, 3, 4, 5, 5, 5, 5 };

enum {
	C,
	E,
	H,
	L,
	O,
	Q,
	N1,
	N2,
	N3,
	N4,
};

const int hello[5] = { H, E, L, L, O };

// 【修正箇所】 index 3 (hugo) を B001(赤) から B100(緑) に変更
const int led[5] = { B000, B001, B010, B100, B111 };

typedef enum {
	WAIT,
	OUT,
} enc_status;
enc_status encst;

typedef enum {
	none,
	ten,
	sen,
	hugo,
	tango,
} Beepst;
Beepst bp;

int tsw, sw;
int pretsw, presw;
bool swps = false;

int idx = 0, i = 0;
bool sounding = false;
word next_dur = 0;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (tsw == HIGH && encst == WAIT && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);

	encst = WAIT;
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;

			swps = false;
		}

		switch (encst) {
		case WAIT:
			bp = none;

			if (swps) {
				swps = false;
				encst = OUT;
				idx = i = 0;
				sounding = false;
				next_dur = 0;
				tc = next_dur;
			}

			break;
		case OUT:
			// 点、線、符号 と符号の間隔、語 と語の間隔 のそれぞれで、赤、青、緑、白 に発光。

			if (tc >= next_dur) {
				tc = 0;

				if (!sounding) {
					int data = morse[hello[idx]][i];
					tone(BZ_PIN, 1000);
					if (data == 1) {
						next_dur = 600;
						bp = sen;
					} else if (data == 0) {
						next_dur = 200;
						bp = ten;
					}
					sounding = true;
				} else {
					noTone(BZ_PIN);
					sounding = false;

					i++;
					if (i >= size[hello[idx]]) {
						i = 0;
						idx++;
						if (idx >= 5) {
							encst = WAIT;
							bp = none;
							break;
						}
						next_dur = 600;
						bp = tango;
					} else {
						next_dur = 200;
						bp = hugo;
					}
				}
			}

			break;
		}
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			noTone(BZ_PIN);
			bp = none;
		}
	}

	lmc = led[(int)bp];
	led_stepmotor(lm.b8);
}