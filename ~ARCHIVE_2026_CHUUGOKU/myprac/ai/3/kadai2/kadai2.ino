#define USE_TIMER3_ISR
#include "mono_con.h"

int x;
int phase = 0;
word tc;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
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

int getidx(int xx) {
	return map(xx, 0, 1023, 0, 3);
}

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

void loop() {
	int i = getidx(x);
	if (tc >= 30) {
		tc = 0;
		if (i == 0) {
			cw();
		} else if (i == 2) {
			ccw();
		}
	}
	disp(0x00, num[phase]);
	led_stepmotor(lm.b8);
}
