#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, x, y;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {

	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	
}
