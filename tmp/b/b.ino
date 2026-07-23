#define USE_TIMER3
#include "mono_con.h"

int sw, presw;
bool swps = false;
bool mv = false;
bool r = true;
word tc = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (r) {
		r = false;
		sw = digitalRead(pin4);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}

	if (swps) {
		swps = false;
		mv = !mv;
	}

	if (mv) {
		if (tc >= 20) {
			tc = 0;
			lm.spm(CW).flush();
		}
	}
}
