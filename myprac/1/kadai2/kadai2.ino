// 16:38

#define USE_TIMER3_ISR
#include "mono_con.h"

const int edge[2] = { 341, 682 };
const int ledParam[9] = { B101, B001, B011, B100, B000, B010, B111, B111, B110 };
const int blink[2] = { B111, B000 };
int blinkidx = 0;

int x, y;
int jsidx;

word in, tc;

int getjsidx(int* x_, int* y_) {
	if (*x_ < edge[0]) {
		if (*y_ < edge[0]) return 6;
		else if (*y_ > edge[1]) return 0;
		else return 3;
	} else if (*x_ > edge[1]) {
		if (*y_ < edge[0]) return 8;
		else if (*y_ > edge[1]) return 2;
		else return 5;
	} else { // 中
		if (*y_ < edge[0]) return 7;
		else if (*y_ > edge[1]) return 1;
		else return 4;
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A2);
		y = analogRead(A1);

		jsidx = getjsidx(&x, &y);
	}

	if (jsidx == 7) tc++;
	else {
		tc = 500;
		blinkidx = 0;
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);

	jsidx = getjsidx(&x, &y);
}

void loop() {
	if (jsidx == 7) {
		if (tc >= 500) {
			tc = 0;
			lm.color.GBR = blink[blinkidx];
			led_stepmotor(lm.b8);
			blinkidx = (blinkidx + 1) % 2;
		}
	} else {
		lm.color.GBR = ledParam[jsidx];
		led_stepmotor(lm.b8);
	}
}