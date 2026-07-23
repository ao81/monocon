#define USE_TIMER3_ISR
#include "mono_con.h"

int value = 0;
word tc;

ISR (TIMER3_COMPA_vect) {
	tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (tc > 1000) {
		tc = 0;
		disp(num[value], num[value]);
		if (++value > 9) value = 0;
	}
}