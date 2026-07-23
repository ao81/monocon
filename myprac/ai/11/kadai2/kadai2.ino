#define USE_TIMER3
#include "mono_con.h"

const int cycle[3] = { 4000, 2000, 1000 };
int cyidx = 1;

word per = 0;

const int color[3] = { B001, B100, B010 };
int coidx = 0;

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool input = true;
word t = 0;
word acc = 0;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 100) {
		in = 0;
		input = true;
	}

	acc += per;
	if (acc >= 100) {
		acc -= 100;
		lm.led(color[coidx]);
	} else lm.led(B000);
	lm.flush();

	if (tsw == LOW) {
		static word slow = 0;
		if (++slow < 20) return;
		slow = 0;

		if (t < cycle[cyidx] / 2) {
			if (t % (cycle[cyidx] / 2 / 100) == 0) {
				per++;
			}
		} else {
			if (t % (cycle[cyidx] / 2 / 100) == 0) {
				per--;
			}
		}
		if (++t >= cycle[cyidx]) t = 0;
	} else {

	}
}

void setup() {
	config_init();
	serial_init();
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
	static int prei = -1;
	if (i != prei) {
		if (prei == -1) {
			if (i == 0) {
				if (cyidx < 2) cyidx++;
				t = acc = per = 0;
			} else if (i == 2) {
				if (cyidx > 0) cyidx--;
				t = acc = per = 0;
			} else if (i == 1) {
				if (++coidx > 2) coidx = 0;
			}
		}
		prei = i;
	}

	if (swps) {
		swps = false;
		cyidx = 1;
		t = acc = per = 0;
	}
}
