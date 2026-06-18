// 28:04

#define USE_TIMER3_ISR
#include "mono_con.h"

int now, phase = 0;
int x;

word in, tc;

int xtoangle(int n) {
	return map(n, 0, 1023, 60, 0);
}

void cw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

void ccw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

void move(int to) {
	if (abs(to - now) <= 0) return;
	int diff = to - now;
	if (diff > 0) {
		cw();
		now++;
	} else {
		ccw();
		now--;
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		x = analogRead(A1);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	now = xtoangle(x);
}

void loop() {
	if (tc > 20) {
		tc = 0;
		move(xtoangle(x));
	}
}