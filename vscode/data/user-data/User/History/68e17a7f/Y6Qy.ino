#define USE_TIMER3_ISR
#include "mono_con.h"

int y;
word in, tc;

int getidx(int n) {
	return map(n, 0, 1023, 0, 3);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		y = analogRead(A2);
	}
}

void setup() {
	config_init();
	serial_init();
	y = analogRead(A2);
}

void loop() {

}