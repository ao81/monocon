// 13:09

// ボリューム0をjs←,→とする
// ボリューム1をjs↑,↓とする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int lo = 0 + 200;
const int hi = 1023 - 200;

const int wait[4] = { 300, 500, 1000, 2000 };
const int cntPara[4] = { -10, -1, 1, 10 };

int waitidx = 0, cntidx = 0;

int tsw, sw, xpos, ypos;
int presw, prexpos, preypos;
int xjsps = 0, yjsps = 0;
bool swps = false;

bool ismove = false;

int count = 0;

word in, tc;

void onedisp(int n) {
	static int pren = -1;
	if (n != pren) {
		pren = n;
		disp(num[n / 10], num[n % 10]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		xpos = analogRead(A1);
		ypos = analogRead(A2);

		if (sw == LOW && presw == HIGH) swps = true;
		if (xpos > hi && prexpos <= hi) xjsps = -1;
		if (xpos < lo && prexpos >= lo) xjsps = 1;
		if (ypos > hi && preypos <= hi) yjsps = -1;
		if (ypos < lo && preypos >= lo) yjsps = 1;

		presw = sw;
		prexpos = xpos;
		preypos = ypos;
	}

	if (ismove) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	xpos = prexpos = analogRead(A1);
	ypos = preypos = analogRead(A2);

	disp(num[0], num[0]);
}

void loop() {
	if (xjsps != 0) {
		if (xjsps == -1) {
			if (cntidx > 0) cntidx--;
		}
		if (xjsps == 1) {
			if (cntidx < 3) cntidx++;
		}
		xjsps = 0;
	}

	if (yjsps != 0) {
		if (yjsps == -1) {
			if (waitidx > 0) waitidx--;
		}
		if (yjsps == 1) {
			if (waitidx < 3) waitidx++;
		}
		yjsps = 0;
	}

	ismove = (tsw == HIGH);

	if (ismove) {
		if (tc > wait[waitidx]) {
			tc = 0;
			count += cntPara[cntidx];
			if (count < 0) count += 100;
			if (count > 99) count -= 100;
		}
	} else {
		tc = 2000;
	}

	onedisp(count);
}