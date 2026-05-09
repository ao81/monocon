#define USE_TIMER3_ISR
#include "mono_con.h"

const int speed[2] = { 20, 40 };
int x, tsw, sw;

word in;

int getidx(int n) {
	return map(n, 0, 1023, 0, 5);
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

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw == HIGH) {
		int i = getidx(x);
		if (i <= 1) {

		} else if (i >= 3) {

		}
	}
}