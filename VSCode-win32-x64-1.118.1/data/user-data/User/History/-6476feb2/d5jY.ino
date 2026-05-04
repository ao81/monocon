#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledpara[3] = { B100, B010, B001 };
int idx = 0;

word tc;

ISR(TIMER3_COMPA_vect) {
	tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (tc > 1000) {
		
	}
}