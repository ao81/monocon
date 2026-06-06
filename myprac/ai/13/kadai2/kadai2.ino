#define USE_TIMER3
#include "mono_con.h"

const int angles[6] = { 0, 20, 40, 60, 80, 100 };
int angle = 0, move = 0;

int x, y;
int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

bool mode = false;
bool tgl = true;
bool mv = false;

int n = 0;

word tc = 0, t = 0;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

int getmove(int from, int to) {
	return to - from;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
	t++;
}

void setup() {
	config_init();
	serial_init();
	tsw = pretsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
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
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (!mode) {
		if (t >= 500) {
			t = 0;
			tgl = !tgl;
		}

		if (!mv) lm.led(tgl ? B101 : B000);
		else lm.led(tgl ? B111 : B000);

		if (swps) {
			swps = false;
			mv = true;
		}

		if (phps) {
			phps = false;
			mv = false;
			mode = true;
		}

		if (mv) {
			if (tc >= 70) {
				tc = 0;
				lm.spm(CW);
			}
		}

		angle = move = 0;
		disp(0x00, 0x00);

	} else {
		int i = getdir(x, y);
		static int prei = -1;
		if (i != prei) {
			if (prei == -1) {
				if (i == 0) { // up
					if (n < 5) n++;
				} else if (i == 2) { // down
					if (n > 0) n--;
				}
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			move = getmove(angle, angles[n]);
		}

		if (tc >= 10) {
			tc = 0;
			if (move > 0) {
				angle++;
				move--;
				lm.spm(CW);
			} else if (move < 0) {
				angle--;
				move++;
				lm.spm(CCW);
			}
		}

		if (move) lm.led(B010);
		else lm.led(B100);

		disp(0x00, num[n]);
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == LOW) {
			mode = false;
			swps = phps = false;
			tgl = true;
			mv = false;
			n = 0;
		}
	}

	lm.flush();
}
