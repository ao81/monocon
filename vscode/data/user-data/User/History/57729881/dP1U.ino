#define USE_TIMER3_ISR
#include "mono_con.h"

int x, tsw, sw;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	
}