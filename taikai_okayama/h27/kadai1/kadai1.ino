#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, xpos;
int pretsw, prexpos;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		xpos = analogRead(A1);
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {

}