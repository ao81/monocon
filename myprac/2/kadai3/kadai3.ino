#define USE_TIMER3_ISR
#include "mono_con.h"

const int speed[3] = { 30, 50, 70 };
int idx = 0;

int x, y;
int phase = 0;

word in, tc;

int postoidx(int p) {
	return map(p, 0, 1023, 0, 6);
}

int getmaxpos(int a, int b) {
	int ta = a, tb = b;
	if (abs(ta - 511) > abs(tb - 511)) return a;
	else return b;
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		y = analogRead(A2);
	}

	tc++;
}

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	y = analogRead(A2);
}

void loop() {
	idx = postoidx(getmaxpos(x, y));

	if (idx < 3) {
		if (tc > speed[2 - idx]) {
			tc = 0;
			cw();
		}
	} else if (idx > 3) {
		if (tc > speed[idx - 4]) {
			tc = 0;
			ccw();
		}
	}
}
