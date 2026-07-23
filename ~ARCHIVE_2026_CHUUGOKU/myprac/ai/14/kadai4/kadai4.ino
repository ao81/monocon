#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
int presw;
bool swps = false;
bool r = true;

int n = 0;
int a = 0, b = 0, ans = 0;

int phase = 0;

bool tgl = true;
word tc = 0;

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

	int i = getdir(x, y);
	static int prei = -1;

	if (i != prei) {
		if (prei == -1) {
			static const int a[4] = { 1, 10, -1, -10 };
			n = constrain(n + a[i], 0, 99);
		}
		prei = i;
	}

	if (swps) {
		swps = false;
		if (++phase > 2) phase = 0;
		if (phase == 2) tone(BZ_PIN, 2000, 50);
		n = 0;
	}

	if (phase == 0) {
		lm.led(B000);
		a = n;
	} else if (phase == 1) {
		lm.led(B111);
		b = n;
	} else {
		if (b == 0) {
			disp(0x79, 0x79);
			if (tc >= 300) {
				tc = 0;
				lm.led(tgl ? B001 : B000);
				tgl = !tgl;
			}

		} else {
			lm.led(tsw ? B100 : B010);
			n = (tsw ? a / b : a % b);
		}
	}

	disp(num[n / 10], num[n % 10]);
	lm.flush();
}
