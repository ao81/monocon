#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledParam[] = { B001, B100, B101, B010, B011, B110, B111 };
int ledIdx = 0;

int tsw, pretsw;

word in, tc;

ISR (TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	lm.color.GBR = B000;
	led_stepmotor(lm.b8);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			tc = 1000;
			ledIdx = 0;
		}

		if (tc > 1000) {
			tc = 0;
			lm.color.GBR = ledParam[ledIdx];
			if (++ledIdx > 6) ledIdx = 0;
		}
	} else {
		if (tsw != pretsw) pretsw = tsw;
		lm.color.GBR = B000;
	}
	led_stepmotor(lm.b8);
}