#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int pretsw, presw;
bool swps = false;
bool r = true;

int n = 0;
bool move = false;
bool wait = false;
bool tgl = true;
bool alarm = false;
word cd;

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
}

void setup() {
	config_init();
	serial_init();
	sw = presw = digitalRead(pin4);
	tsw = pretsw = digitalRead(pin5);
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

	if (!move) {
		lm.led(B010);

		int i = getdir(x, y);
		static int prei = -1;
		if (i != prei) {
			if (prei == -1) {
				if (i == 0) n++;
				else if (i == 1) n += 10;
				else if (i == 2) n--;
				else n -= 10;
				n = constrain(n, 0, 99);
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			move = true;
			cd = 0;
			wait = false;
			tgl = true;
			alarm = false;
		}

	} else {
		if (alarm) {
			if (cd <= 0) {
				cd = 500;
				lm.led(tgl ? B001 : B000);
				tgl = !tgl;
			}
		} else {
			lm.led(B100);

			if (!wait && cd <= 0) {
				cd = 1000;
				if (n > 0) n--;
				if (n > 0 && n <= 5) tone(BZ_PIN, 2000, 50);
				else if (n == 0) {
					alarm = true;
					tone(BZ_PIN, 2000);
					cd = 0;
				}
			}
		}

		if (swps) {
			swps = false;
			if (n > 0) {
				wait = !wait;
				noTone(BZ_PIN);
			} else {
				move = false;
				noTone(BZ_PIN);
			}
		}
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == LOW) {
			move = false;
			n = 0;
			noTone(BZ_PIN);
		}
	}

	disp(num[n / 10], num[n % 10]);
	lm.flush();
}
