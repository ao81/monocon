#define USE_TIMER3
#include "mono_con.h"

const int level[6] = { 0, 20, 40, 60, 80, 100 };

int x, y, tsw, sw, ph;
bool r = true;

int idx = 0;

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

	static int acc = 0;
	acc += level[idx];
	if (acc >= 100) {
		acc -= 100;
		lm.led(B111);
	} else lm.led(B000);
	lm.flush();
}

void setup() {
	config_init();
	serial_init();

	disp(0x00, num[0]);
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
			if (i == 0) {
				if (idx < 5) idx++;
			} else if (i == 2) {
				if (idx > 0) idx--;
			}
			disp(0x00, num[idx]);
		}
		prei = i;
	}
}
