#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

bool du = false;
int n = 0;

word tc = 0, cd = 0;

int getdir(int xx, int yy) {
	long dx = xx - 511, dy = yy - 511;
	if (dx * dx + dy * dy < 20000) return -1;
	return (int)((atan2(dx, dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (++in > 5) {
		in = 0;
		r = true;
	}

	if (tc > 0) {
		tc--;
		if (tc == 0) {
			noTone(BZ_PIN);
			lm.led(B000);
		}
	}
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;

		y = 1023 - analogRead(pin2);
		x = 1023 - analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	int i = getdir(x, y);
	static int prei = -1;

	if (i != prei) {
		if (!tsw) {
			if (prei == -1) {
				if (i == 0 || i == 2) {
					cd = 500;
					du = true;
				}
				if (i == 1 || i == 3) du = false;
			} else if (i == -1) {
				if (du) {
					du = false;
					if (cd > 0) {
						if (prei == 0) n++;
						else n--;
						tone(BZ_PIN, 2000);
						lm.led(B100);
						tc = 50;
					} else {
						if (prei == 0) n += 10;
						else n -= 10;
						tone(BZ_PIN, 800);
						lm.led(B010);
						tc = 50;
					}
					n = constrain(n, 0, 99);
				}
			}
		}

		prei = i;
	}

	if (swps) {
		swps = false;
		n = 0;
	}

	disp(num[n / 10], num[n % 10]);
	lm.flush();
}
