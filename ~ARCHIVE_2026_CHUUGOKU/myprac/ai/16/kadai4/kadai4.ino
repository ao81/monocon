#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

int n = 0, m = 0;
int cnt = 0;
word tc = 0, cd = 0;

typedef enum {
	wait,
	cw,
	ccw,
	res,
} status;
status st = wait;

int getdir(int xx, int yy) {
	long dx = xx - 511, dy = yy - 511;
	if (dx * dx + dy * dy < 20000) return -1;
	return (int)((atan2(dx, dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (++in > 5) {
		in = 0;
		r = true;
	}

	if (cd > 0) {
		cd--;
		if (cd == 0) lm.led(B000);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();
	tsw = pretsw = digitalRead(pin3);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;

		y = 1023 - analogRead(pin2);
		x = 1023 - analogRead(pin1);
		tsw = digitalRead(pin3);
		sw = digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	int i = getdir(x, y);
	static int prei = -1;

	if (m >= 8) {
		lm.led(B100);


	} else if (i != prei) {
		if (i != -1) {
			if (i == n) {
				if (!tsw) {
					if (++n > 7) n = 0;
				} else {
					if (--n < 0) n = 7;
				}

				if (m < 8) {
					m++;
					if (m == 8) cnt = 2;
				}

				tone(BZ_PIN, 1000, 50);
			} else {
				n = m = 0;
				lm.led(B001);
				cd = 500;
				tone(BZ_PIN, 800, 500);
			}
		}
		prei = i;
	}

	if (tc >= 400) {
		tc = 0;
		if (cnt > 0) {
			tone(BZ_PIN, 2000, 200);
			cnt--;
		}
	}

	if (swps) {
		swps = false;
		n = m = 0;
		lm.led(B000);
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		n = m = 0;
		lm.led(B000);
		noTone(BZ_PIN);
	}

	disp(tsw ? 0x5e : 0x58, num[m]);
	lm.flush();
}
