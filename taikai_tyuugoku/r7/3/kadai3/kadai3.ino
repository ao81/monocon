// 途中まで

#define USE_TIMER3_ISR
#include "mono_con.h"

const int angles[9] = { 0, 15, 30, 45, 60, 75, 90, 105, 120 };
int idx = 4;
int x, y;


ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		x = analogRead(A2);
		y = analogRead(A1);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);
}

void loop() {

}
