#define USE_TIMER3
#include "mono_con.h"

int tsw, sw;
word tc;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin5);
	sw = digitalRead(pin4);
}

void loop() {
	if (!sw) {
		if (tc >= 30) {
			tc = 0;

			if (tsw == HIGH) {
				lm.led(B100).spm(CW);
			} else {
				lm.led(B010).spm(CCW);
			}
		}
	} else {
		lm.led(B001).spm(STOP);
	}

	lm.flush();
}
