// 27:08
// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"

const double step_vol = 90.0 / 99.0;

int sw, ph;
int vol = 0;
int phase = 0;
double angle = 0.0;
int preangle = 0;

word in, tc;

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

		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tc > 40) {
		tc = 0;
		if (sw == LOW) {
			if (vol < 99) {
				vol++;
				angle += step_vol;
			}

		} else if (ph == HIGH) {
			if (vol > 0) {
				vol--;
				angle -= step_vol;
			}
		}

		int cur = (int)angle;
		int diff = cur - preangle;
		if (diff > 0) {
			for (int i = 0; i < diff; i++) cw();
		} else if (diff < 0) {
			for (int i = 0; i < -diff; i++) ccw();
		}
		preangle = cur;
	}
	disp(num[vol / 10], num[vol % 10]);
}
