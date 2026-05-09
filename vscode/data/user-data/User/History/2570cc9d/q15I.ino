#define USE_TIMER3_ISR
#include "mono_con.h"

int sw, presw;
bool swps = false;

word in;

ISR(TIMER_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (swps) {
		swps = false;
		
	}
}
