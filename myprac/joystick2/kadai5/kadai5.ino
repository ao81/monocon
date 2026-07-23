#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
bool r = true;

int n = 0;

word tc;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();
	disp(num[0], num[0]);
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

	int i = getdir(x, y);
	if (tc >= 500) {
		tc = 0;
		if (i == 0 || i == 1) {
			if (++n > 99) n = 0;
		} else if (i == 2 || i == 3) {
			if (--n < 0) n = 99;
		} else tc = 500;
	}

	disp(num[n / 10], num[n % 10]);
}
