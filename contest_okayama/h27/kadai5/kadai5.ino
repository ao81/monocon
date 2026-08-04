// 15:57

// ボリュームをjs← →とし、sw2をjs↓とする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int lo = 0 + 200;
const int hi = 1023 - 200;

const int wt[4] = { 100, 500, 1000, 2000 };
int wtidx = 0;

int tsw, sw, xjs, yjs;
int presw, prexjs, preyjs;
int xjsps = 0, dir = 1;
bool swps = false, jsps = false;

int count = 0;

word in = 0, tc = 0;

typedef enum {
	COUNT,
	STOP
} Status;
Status st = COUNT;

void onedisp(int n) {
	static int pren = -1;
	if (n != pren) {
		pren = n;
		disp(num[n / 16], num[n % 16]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		xjs = analogRead(A1);
		yjs = analogRead(A2);

		if (sw == LOW && presw == HIGH) swps = true;
		if (yjs > hi && preyjs <= hi) jsps = true;
		if (xjs > hi && prexjs <= hi) xjsps = -1;
		if (xjs < lo && prexjs >= lo) xjsps = 1;

		presw = sw;
		preyjs = yjs;
		prexjs = xjs;
	}

	if (st == COUNT) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	xjs = prexjs = analogRead(A1);
	yjs = preyjs = analogRead(A2);

	disp(num[0], num[0]);
}

void loop() {
	if (swps) {
		swps = false;
		if (st == COUNT) st = STOP;
		else st = COUNT;
	}

	if (jsps) {
		jsps = false;
		count = 0;
	}

	if (xjsps != 0) {
		if (xjsps == 1) {
			if (wtidx < 3) wtidx++;
		} else {
			if (wtidx > 0) wtidx--;
		}
		xjsps = 0;
	}

	if (tsw == HIGH) dir = -1;
	else dir = 1;

	switch (st) {
	case COUNT:
		if (tc > wt[wtidx]) {
			tc = 0;
			count += dir;
			if (count < 0) count = 255;
			else if (count > 255) count = 0;
		}

		break;
	case STOP:
		break;
	}

	onedisp(count);
}