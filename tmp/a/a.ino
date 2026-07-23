#define USE_TIMER3
#include "mono_con.h"

int sw;
int phase = 0;
word tc = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		sw = digitalRead(pin4);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();
	sw = digitalRead(pin4);
}

void loop() {
	if (!sw) {
		if (tc >= 20) {
			tc = 0;
			if (--phase < 0) phase = 3;
			lm.spm(phase);
		}
		lm();
	} else tc = 20;
}
