#define USE_TIMER3
#include "mono_con.h"

const int ans[5] = { 1, 0, 1, 1, 0 };

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool r = true;

bool game = false;
int n = 1;
int ok = 0;
int select = -1;

int cnt = 2;

word tc, cd;

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
	if (cd > 0) cd--;
	tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin4);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	if (!game) {
		if (swps) {
			swps = false;
			game = true;
			n = 1;
			ok = 0;
			select = -1;
			cnt = 2;
		}

		disp(0x00, 0x00);

	} else if (n <= 5) {
		disp(num[n], num[ok]);

		int i = getdir(x, y);
		static int prei = -1;

		if (i != prei) {
			if (prei == -1) {
				if (i == 0) {
					select = 1;
					lm.led(B100);
					cd = 100;
				} else if (i == 2) {
					select = 0;
					lm.led(B001);
					cd = 100;
				}
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			if (select != -1) {
				if (select == ans[n - 1]) {
					ok++;
					tone(BZ_PIN, 2000, 60);
				} else tone(BZ_PIN, 800, 60);
				n++;
			}
		}

		if (cd == 0) lm.led(B000);

	} else {
		if (ok == 5) lm.led(B111);
		else if (ok >= 3) lm.led(B100);
		else lm.led(B001);

		if (tc >= 200) {
			tc = 0;
			if (cnt-- > 0) {
				tone(BZ_PIN, 2000, 100);
			}
		}

		if (swps) {
			swps = false;
			game = false;
			lm.led(B000);
		}

		disp(num[5], num[ok]);
	}

	lm.flush();
}
