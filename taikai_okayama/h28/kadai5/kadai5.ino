// 21:18

// 周期の時間のled表示を、赤,黄,青,緑,紫　とする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int lo = 0 + 200;
const int hi = 1023 - 200;

const int ledPara[5] = { B001, B101, B010, B100, B011 };
int ledidx = 0;

const int segPara[12][2] = {
	{ 0x04, 0x20 },
	{ 0x00, 0x21 },
	{ 0x00, 0x03 },
	{ 0x00, 0x06 },
	{ 0x00, 0x0C },
	{ 0x00, 0x18 },
	{ 0x02, 0x10 },
	{ 0x03, 0x00 },
	{ 0x21, 0x00 },
	{ 0x30, 0x00 },
	{ 0x18, 0x00 },
	{ 0x0C, 0x00 },
};
int segidx = 0;

int sw, ph, tsw, xpos;
int presw, preph, prexpos;

int wait = 1000;
word in, tc;

bool first = true;

typedef enum {
	STOP,
	MOVE,
	INIT
} Status;
Status st;

void onedisp(int n[2]) {
	static int pren[2] = { 0x00, 0x00 };
	if (memcmp(n, pren, sizeof(pren)) != 0) {
		memcpy(pren, n, sizeof(pren));
		disp(n[0], n[1]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
		tsw = digitalRead(_USER_CON_5PIN);
		xpos = analogRead(A1);
	}

	if (st == MOVE) tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
	tsw = digitalRead(_USER_CON_5PIN);
	xpos = prexpos = analogRead(A1);

	onedisp(segPara[0]);
}

void loop() {
	if (ph == HIGH && preph == LOW) {
		st = INIT;
	}

	if (xpos > hi && prexpos <= hi) {
		if (ledidx > 0) ledidx--;
		wait = (ledidx + 1) * 1000;
		tc = 0;
	}

	if (xpos < lo && prexpos >= lo) {
		if (ledidx < 4) ledidx++;
		wait = (ledidx + 1) * 1000;
		tc = 0;
	}

	switch (st) {
	case STOP:
		if (sw == LOW && presw == HIGH) {
			st = MOVE;
		}

		break;
	case MOVE:
		if (sw == LOW && presw == HIGH) {
			st = STOP;
		}

		if (tc > wait) {
			tc = 0;
			if (tsw == LOW) {
				if (++segidx > 11) segidx = 0;
			} else {
				if (--segidx < 0) segidx = 11;
			}
		}

		break;
	case INIT:
		segidx = 0;
		st = STOP;

		break;
	}

	presw = sw;
	preph = ph;
	prexpos = xpos;

	lm.color.GBR = ledPara[ledidx];
	led_stepmotor(lm.b8);

	onedisp(segPara[segidx]);
}