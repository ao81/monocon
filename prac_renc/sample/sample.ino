#define USE_TIMER3
#include "mono_con.h"

int ts, sw, ph;
int a, b;
int pa, pb;
bool r = true;

ISR(TIMER3_COMPA_vect) { // 0.1ms
	static word in = 0;
	if (in++ > 50) {
		in = 0;
		r = true;
	}

	a = digitalRead(pin1);
	b = digitalRead(pin2);
}

void setup() {
	config_init();
	serial_init();

	a = pa = digitalRead(pin1);
	b = pb = digitalRead(pin2);
}

void loop() {
	if (r) {
		r = false;
		ts = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);
	}

	static int c = 0;

	if (pa == 0 && a == 1) {
		if (b == 1) c--;
		else c++;
	}
	pa = a;
	pb = b;

	if (c < 0) c += 24;
	if (c > 23) c -= 24;

	disp(num[c / 10], num[c % 10]);
}
