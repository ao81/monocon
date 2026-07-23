#define USE_TIMER3_ISR
#include "mono_con.h"

const int speed = 30;
int phase = 3;
word tc;

ISR (TIMER3_COMPA_vect) {
	tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (tc > speed) {
		tc = 0;

		lm.color.SM = stepm_init(phase);
		led_stepmotor(lm.b8);

		if (--phase < 0) phase = 3;
	}
}