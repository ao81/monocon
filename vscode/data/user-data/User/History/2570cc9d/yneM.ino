#define USE_TIMER3_ISR
#include "mono_con.h"

int sw, presw;
bool swps = false;

word in;

ISR(TIMER_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw= digitalRead(_USER_CON_4PIN);
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	
}
