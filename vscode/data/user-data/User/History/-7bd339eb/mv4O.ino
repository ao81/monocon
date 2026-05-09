#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, x;

word in;

typedef enum {
	
} Status;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		x = analogRead(A1);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	x = analogRead(A1);
}

void loop() {

}
