#define USE_TIMER3_ISR
#include "mono_con.h"

int x, y;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		x = analogRead(A1);
		y = analogRead(A2);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	y = analogRead(A2);
}

const int led[5] = { B000, B001, B100, B010, B011 };

int getidx(int xx, int yy) {
	const int dz = 300;
	int xdist = xx - 511;
	int ydist = yy - 511;

	if (abs(xdist) <= dz && abs(ydist) <= dz) return 0;
	if (abs(xdist) > abs(ydist)) return (xdist < 0) ? 4 : 3;
	else return (ydist < 0) ? 1 : 2;
}

void loop() {
	lm.color.GBR = led[getidx(x, y)];
	led_stepmotor(lm.b8);
}
