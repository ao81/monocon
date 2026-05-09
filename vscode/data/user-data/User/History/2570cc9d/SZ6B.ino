#define USE_TIMER3_ISR
#include "mono_con.h"

word in;

ISR(TIMER_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

	}
}

void setup() {
	config_init();
	serial_init();

	Serial.begin(9600);
}

void loop() {
	
}