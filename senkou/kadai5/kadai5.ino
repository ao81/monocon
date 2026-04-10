#define USE_TIMER3_ISR
#include "mono_con.h"

const int lowEdge = 0 + 200;

int sw, ypos;

int count = 0;

word in, tc;

void onedisp(int n) {
	static int pren = -1;
	if (pren != n) {
		pren = n;
		disp(num[n / 10], num[n % 10]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ypos = analogRead(A2);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = digitalRead(_USER_CON_4PIN);
	ypos = analogRead(A2);

	disp(num[0], num[0]);
}

void loop() {
	if (sw == LOW) {
		if (tc > 500) {
			tc = 0;
			if (++count > 99) count = 0;
		}
	} else if (ypos < lowEdge) {
		if (tc > 500) {
			tc = 0;
			if (--count < 0) count = 99;
		}
	}
	onedisp(count);
}