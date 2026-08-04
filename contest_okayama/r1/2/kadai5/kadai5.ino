// 7:55

#define USE_TIMER3_ISR
#include "mono_con.h"

const int lowEdge = 0 + 200;

int sw, ypos;
int presw, preypos;
bool sw1ps = false, sw2ps = false;

int count = 0;

word in, tc;

void onedisp(int n) {
	static int pren = -1;
	if (pren != n) {
		pren = n;
		disp(num[n / 10], num[n % 10]);
	}
}

ISR (TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ypos = analogRead(A2);

		if (sw == LOW && presw == HIGH) sw1ps = true;
		if (ypos < lowEdge && preypos >= lowEdge) sw2ps = true;
		
		presw = sw;
		preypos = ypos;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);
	ypos = preypos = analogRead(A2);

	disp(num[0], num[0]);
}

void loop() {
	if (sw1ps || sw2ps) {
		sw1ps = sw2ps = false;
		tc = 500;
	}

	if (sw == LOW) {
		if (tc > 500) {
			tc = 0;
			if (++count > 99) count = 0;
		}
	}

	if (ypos < lowEdge) {
		if (tc > 500) {
			tc = 0;
			if (--count < 0) count = 99;
		}
	}

	onedisp(count);
}