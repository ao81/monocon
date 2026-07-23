// 4:29

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn[7] = { B001, B100, B101, B010, B110, B011, B111 };
int idx = 0;

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
}

void loop() {
	if (tsw == HIGH) {
		if (pretsw != tsw) {
			pretsw = tsw;
			tc = 1000;
			idx = 0;
		}

		if (tc > 1000) {
			tc = 0;
			lm.color.GBR = ptn[idx];
			if (++idx > 6) idx = 0;
		}
	} else {
		if (pretsw != tsw) pretsw = tsw;
		lm.color.GBR = B000;
	}
	led_stepmotor(lm.b8);
}