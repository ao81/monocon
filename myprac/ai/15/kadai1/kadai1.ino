#define USE_TIMER3
#include "mono_con.h"

const int a[4] = { 1, 10, -1, -10 };

int x, y, tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;
bool r = true;

int k = 0;
int n = 0;
int res = 0;

bool dp = false;

typedef enum {
	key,
	number,
	wait,
	result,
} status;
status st = key;

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
		if (ph == LOW && preph == HIGH) phps = true;

		presw = sw;
		preph = ph;
	}

	int i = getdir(x, y);
	static int prei = -1;

	switch (st) {
	case key:
		if (i != prei) {
			if (prei == -1) {
				k = constrain(k + a[i], 0, 99);
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			st = number;
			n = 0;
		}

		disp(num[k / 10], num[k % 10]);

		break;
	case number:
		if (i != prei) {
			if (prei == -1) {
				n = constrain(n + a[i], 0, 99);
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			st = result;
			dp = false;
			phps = false;
		}

		disp(num[n / 10], num[n % 10]);

		break;
	case result:
		lm.led(tsw ? B010 : B100);

		if (swps) {
			swps = false;
			if (!dp) {
				dp = true;
				tone(BZ_PIN, 2000, 50);
			} else {
				st = number;
			}
		}

		if (phps) {
			phps = false;
			st = key;
		}

		if (dp) {
			if (!tsw) res = (n + k) % 100;
			else res = (n - k + 100) % 100;

			disp(num[res / 10], num[res % 10]);
		} else disp(0x00, 0x00);

		break;
	}

	lm.flush();
}
