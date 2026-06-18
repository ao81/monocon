#define USE_TIMER3
#include "mono_con.h"

int save = 0;

int x, y, tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;
bool r = true;

int n = 0, m = 0;
bool sele = true;

bool tgl = true;
word tc = 0;

int getdir(int xx, int yy) {
	long dx = xx - 511, dy = yy - 511;
	if (dx * dx + dy * dy < 20000) return -1;
	return(int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (++in > 5) {
		in = 0;
		r = true;
	}
	tc++;
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

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	int i = getdir(1023 - y, 1023 - x);
	static int prei = -1;

	if (i != prei) {
		if (!tsw && prei == -1) {
			if (i == 3) {
				sele = false;
			} else if (i == 1) {
				sele = true;
			} else if (i == 0) {
				if (sele) {
					if (++m > 9) m = 0;
				} else {
					if (++n > 9) n = 0;
				}
			} else if (i == 2) {
				if (sele) {
					if (--m < 0) m = 9;
				} else {
					if (--n < 0) n = 9;
				}
			}

			tgl = true;
			tc = 500;
		}
		prei = i;
	}

	if (tc >= 500) {
		tc = 0;
		if (tgl) {
			disp(num[n], num[m]);
		} else {
			if (sele) disp(num[n], 0x00);
			else disp(0x00, num[m]);
		}
		tgl = !tgl;
	}

	if (swps) {
		swps = false;
		save = n * 10 + m;
		tgl = true;
		tc = 500;
		tone(BZ_PIN, 2000, 50);
	}

	if (phps) {
		phps = false;
		n = save / 10;
		m = save % 10;
		tgl = true;
		tc = 500;
		tone(BZ_PIN, 1000, 50);
	}

	if (sele) lm.led(B100);
	else lm.led(B010);
	lm.flush();
}
