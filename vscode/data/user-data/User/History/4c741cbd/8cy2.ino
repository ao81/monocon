#define USE_TIMER3_ISR
#include "mono_con.h"

const int bz[8] = { 262, 294, 330, 349, 392, 440, 494, 523 };
enum { C4, D4, E4, F4, G4, A4, B4, C5 ,C4 };

int tsw;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
	}
}

void setup() {
	config_init();
	serial_init();
	tsw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (tsw == HIGH) {

	}
}