#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool input = true;

int a = 0, b = 0, op = 0, res = 0;

typedef enum {
	A,
	B,
	RES,
} status;
status st = A;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
	}
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin4);
}

void loop() {
	if (input) {
		input = false;
		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	int i = getdir(x, y);
	static int prei = -2;

	if (st != RES) {
		op = (int)tsw;
		if (op == 0) lm.led(B100);
		else lm.led(B001);
		lm.flush();
	}

	switch (st) {
	case A:
		if (i != prei) {
			if (prei == -1) {
				if (i == 0) a++;
				else if (i == 1) a += 10;
				else if (i == 2) a--;
				else a -= 10;
				a = constrain(a, 0, 99);
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			st = B;
		}

		disp(num[a / 10], num[a % 10]);

		break;
	case B:
		if (i != prei) {
			if (prei == -1) {
				if (i == 0) b++;
				else if (i == 1) b += 10;
				else if (i == 2) b--;
				else b -= 10;
				b = constrain(b, 0, 99);
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			res = (op ? a - b : a + b);
			res = abs(res);
			res %= 100;
			st = RES;
		}

		disp(num[b / 10], num[b % 10]);

		break;
	case RES:
		disp(num[res / 10], num[res % 10]);

		if (swps) {
			swps = false;
			disp(num[0], num[0]);
			a = b = 0;
			st = A;
		}

		break;
	}
}
