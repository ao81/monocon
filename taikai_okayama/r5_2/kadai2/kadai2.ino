// 15:21

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn[3][6] = {
	{ 1,1,1,0,0,0 },
	{ 0,1,1,1,0,0 },
	{ 0,0,1,1,1,0 },
};

int n;
int tsw, sw, ph;
int presw;
int cycle = 0, idx = 0;
bool working = false, swps = false;

int in = 0, tc = 0;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (tsw == LOW && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (tsw == LOW) tc++;
	else {
		cycle = 0;
		idx = 0;
		tc = 200;
		working = false;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		n = B000;
		if (sw == LOW) n |= B001;
		if (ph == HIGH) n |= B100;
		if (sw == LOW && ph == HIGH) n |= B010;
	} else {
		if (!working) {
			if (swps) {
				swps = false;
				working = true;
				cycle = 0;
				idx = 0;
				tc = 200;
				n = B000;
			}
		} else {
			if (tc >= 200) {
				tc = 0;
				if (ptn[0][idx]) n |= B001;
				else n &= B110;
				if (ptn[1][idx]) n |= B100;
				else n &= B011;
				if (ptn[2][idx]) n |= B010;
				else n &= B101;
				if (++idx > 5) {
					idx = 0;
					cycle++;
				}
				if (cycle > 5) working = false;
			}
		}
	}

	lm.color.GBR = n;
	led_stepmotor(lm.b8);
}