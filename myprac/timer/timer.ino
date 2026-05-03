#define USE_TIMER3_ISR
#include "mono_con.h"

int x, phase = 0;

word in, tc;
int speed = 30;

int getdir(int p) {
	return map(p, 0, 1023, 0, 3);
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
}

void loop() {
	static int predir = -1;
	int dir = getdir(x);
	if (predir != dir) {
		predir = dir;
		if (dir == 0) {
			speed++;
		} else if (dir == 2) {
			speed--;
		}
	}

	if (tc >= speed) {
		tc = 0;
		if (++phase > 3) phase = 0;
		lm.color.SM = stepm_init(phase);
		led_stepmotor(lm.b8);
	}

	disp(num[speed / 10], num[speed % 10]);
}