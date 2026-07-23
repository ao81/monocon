#define USE_TIMER3
#include "mono_con.h"

const int led[8] = { B001, B011, B010, B110, B111, B111, B100, B101 };
int x, y, tsw, sw, ph;
bool r = true;
word tc;
bool tgl = true;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
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

	if (i != -1) {
		if (i == 4) { // 点滅
			if (tc >= 300) {
				tc = 0;
				lm.led(tgl ? B111 : B000);
				tgl = !tgl;
			}
		} else {
			lm.led(led[i]);
			tc = 300;
			tgl = true;
		}
	} else lm.led(B000);

	lm.flush();
}
