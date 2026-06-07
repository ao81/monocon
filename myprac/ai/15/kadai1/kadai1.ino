// 途中まで

#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
bool r = true;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI / PI / 4) / (PI / 2)) % 4;
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

	int i = getdir(x, y);
	static int prei = -1;
	if (i != prei) {
		if (prei == -1) {
			static const int a[4] = { 1, 10, -1, -10 };
		}
		prei = i;
	}
}
