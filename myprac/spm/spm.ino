#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw;
word tc;

void cw() {
	static int phase = 0;
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
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
}

void loop() {
	if (tsw && tc >= 30) {
		tc = 0;
		cw();
	}
}
