#define USE_TIMER3_ISR
#include "mono_con.h"

const int angles[9] = { 75, 60, 45, 90, -1, 30, 105, 0, 15 };
int angle = 0, phase = 0;
int x, y;
word tc;

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

int getidx(int p) {
	return constrain(map(p, 0, 1023, 1, 4), 1, 3);
}

int getdir(int from, int to) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(A2);
		y = analogRead(A1);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);
}

void loop() {
	int xidx = getidx(x);
	int yidx = getidx(y);
	int idx = (xidx - 1) + (yidx - 1) * 3;

	disp(num[xidx], num[yidx]);

	static int to = 0;
	if (angles[idx] != -1) to = getdir(angle, angles[idx]);
	else to = 0;

	if (tc >= 8) {
		tc = 0;

		if (to > 0) {
			cw();
			angle++;
			to--;
		} else if (to < 0) {
			ccw();
			angle--;
			to++;
		}

		angle = (angle + 120) % 120;
	}

	if (angle == 0) lm.color.GBR = B100;
	else if (to != 0) lm.color.GBR = B001;
	else {
		bool ok = false;
		for (int i = 0; i < 9; i++) if (angles[i] == angle) {
			ok = true;
			break;
		}
		if (ok) lm.color.GBR = B010;
		else lm.color.GBR = B000;
	}

	led_stepmotor(lm.b8);
}
