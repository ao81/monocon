#define USE_TIMER3_ISR
#include "mono_con.h"

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