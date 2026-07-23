#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledpara[9] = { B101, B001, B011, B100, B000, B010, B111, B111, B110 };
const int blinkpara[2] = { B111, B000 };

int x, y, idx, bidx;

word in, tc;

int pivpos(int p) {
	return map(p, 0, 1023, 0, 3);
}

int rev(int n) {
	return map(n, 0, 2, 2, 0);
}

int postoidx(int px, int py) {
	int col = rev(pivpos(px));
	int row = pivpos(py);
	return row * 3 + col;
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		y = analogRead(A2);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	y = analogRead(A2);
}

void loop() {
	idx = postoidx(x, y);
	if (idx != 7) {
		lm.color.GBR = ledpara[idx];
		bidx = 0, tc = 500;
	} else {
		if (tc >= 500) {
			tc = 0;
			lm.color.GBR = blinkpara[bidx];
			if (++bidx > 1) bidx = 0;
		}
	}
	led_stepmotor(lm.b8);
}
