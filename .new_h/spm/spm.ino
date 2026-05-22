#define USE_TIMER3
#include "mono_con.h"

int tsw;
int phase = 0;
word tc;

void cw() {
	if (--phase < 0) phase = 3;
	lm.spm(phase).flush();
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(pin_5);
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin_5);
}

void loop() {
	if (tsw && tc >= 30) {
		tc = 0;
		cw();
	}
}
