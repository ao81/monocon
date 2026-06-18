#define USE_TIMER3
#include "mono_con.h"

const int led[9] = { 0, 13, 25, 38, 50, 63, 75, 88, 100 };
int lidx = 5;

const int color[3] = { B001, B100, B010 };
int cidx = 0;

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
int input = true;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word acc = 0;
	acc += led[lidx];
	if (acc >= 100) {
		acc -= 100;
		lm.led(color[cidx]);
	} else lm.led(B000);
	lm.flush();
	disp(0x00, num[lidx]);

	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
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

	if (swps) {
		swps = false;
		lidx = 5;
		cidx = 0;
	}

	static int prei = -1;
	int i = getdir(x, y);
	if (prei != i) {
		if (prei == -1) {
			if (i == 0) {
				if (lidx < 8) lidx++;
			} else if (i == 2) {
				if (lidx > 0) lidx--;
			} else if (i == 1) {
				if (++cidx > 2) cidx = 0;
			}
		}
		prei = i;
	}
}
