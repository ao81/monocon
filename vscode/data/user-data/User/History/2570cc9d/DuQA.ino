#define USE_TIMER3_ISR
#include "mono_con.h"

volatile int sw, presw;
bool swps = false;

volatile word in, tc;

ISR(TIMER_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}

	if (tc > 0) tc--;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (swps) {
		swps = false;
		tc = 1000;
	}

	if (tc > 0) {
		lm.color.GBR = B011;
	} else {
		lm.color.GBR = B000;
	}

	led_stepmotor(lm.b8);
}
