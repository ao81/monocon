#define USE_TIMER3
#include "mono_con.h"

int tsw;
word tc;

void cw() {
	static int phase = 0;
	if (--phase < 0) phase = 3;
	lm.spm(phase).flush();
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(pin5);
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin5);
}

void loop() {
	if (tsw && tc >= 30) {
		tc = 0;
		cw();
	}
}
