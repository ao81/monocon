#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn1[5][2] = {
	{ 0x30, 0x00 },
	{ 0x06, 0x00 },
	{ 0x00, 0x30 },
	{ 0x00, 0x06 },
	{ 0x00, 0x00 },
};

const int ptn2[4][2] = {
	{ 0x01, 0x01 },
	{ 0x40, 0x40 },
	{ 0x08, 0x08 },
	{ 0x00, 0x00 },
};

int tsw, sw1, sw2;
int pretsw, presw1, presw2;

bool sw1press = false, sw2press = false;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1press = true;
		if (sw2 == LOW && presw2 == HIGH) sw2press = true;

		presw1 = sw1;
        presw2 = sw2;
	}

	in++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_4PIN);
	sw1 = presw1 = digitalRead(_USER_CON_5PIN);
	sw2 = presw2 = digitalRead(_USER_CON_3PIN);
}

void loop() {
	static int idx1 = 4, idx2 = 3;
	bool update = false;

	if (tsw == HIGH) { // タクトスイッチ
		if (sw1press) {
			sw1press = false;
			if (--idx1 < 0) idx1 = 3;
			update = true;

		}
		if (sw2press) {
			sw2press = false;
			if (++idx1 > 3) idx1 = 0;
			update = true;

		}
	} else { // ロータリーエンコーダ
		if (sw1press) { // ccw
			sw1press = false;
			if (--idx2 < 0) idx2 = 2;
			update = true;

		}
		if (sw2press) { // cw
			sw2press = false;
			if (++idx2 > 2) idx2 = 0;
			update = true;

		}
	}

	if (update) {
		update = false;

		disp(ptn1[idx1][0] | ptn2[idx2][0], ptn1[idx1][1] | ptn2[idx2][1]);
	}
}