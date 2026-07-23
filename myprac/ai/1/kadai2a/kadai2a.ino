#define USE_TIMER3_ISR
#include "mono_con.h"

const int wait[3] = { 3000, 1000, 3000 };
const int led[3] = { B100, B101, B001 };
int idx = 0;

int pretsw;
volatile int tsw;
volatile word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	pretsw = !tsw;
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			idx = 0;
			tc = 0;
			lm.color.GBR = led[idx];
		}

		if (tc >= wait[idx]) {
			tc = 0;
			if (++idx > 2) idx = 0;
			lm.color.GBR = led[idx];
		}

	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			lm.color.GBR = B000;
		}
	}

	led_stepmotor(lm.b8);
}
