#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
bool r = true;

int getdir(int p) {
	return constrain(map(p, 0, 1023, 0, 10), 0, 9);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);
	}

	disp(num[getdir(x)], num[getdir(y)]);
}
