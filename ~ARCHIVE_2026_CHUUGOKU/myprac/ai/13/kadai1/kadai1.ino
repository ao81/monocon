#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool r = true;

int a = 0, b = 0;
int *p = &a;
int blank = -1;

int phase = 0;

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
		presw = sw;
	}

	if (swps) {
		swps = false;
		if (++phase > 2) {
			phase = 0;
			a = b = 0;
		}
		if (phase == 2) {
			if (a > b) {
				lm.led(B001);
				tone(BZ_PIN, 2000, 1000);
				p = &a;
			} else if (a < b) {
				lm.led(B010);
				tone(BZ_PIN, 1400, 1000);
				p = &b;
			} else {
				lm.led(B100);
				tone(BZ_PIN, 800, 1000);
				p = &blank;
			}
		}
	}

	if (phase == 0) {
		p = &a;
		lm.led(B000);
	} else if (phase == 1) {
		p = &b;
		lm.led(B111);
	}

	int i = getdir(x, y);
	static int prei = -1;
	if (i != prei) {
		if (prei == -1) {
			static const int d[] = { 1, 10, -1, -10 };
			*p = constrain(*p + d[i], 0, 99);
		}
		prei = i;
	}

	if (*p != -1) disp(num[*p / 10], num[*p % 10]);
	else disp(0x00, 0x00);
	lm.flush();
}
