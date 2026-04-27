// 7:52

#define USE_TIMER3_ISR
#include "mono_con.h"

const int l = 0 + 200;
const int h = 1023 - 200;

int x, y;
bool add = false, sub = false;

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
		x = analogRead(A1);
		y = analogRead(A2);

		add = (x < l || y < l);
		if (!add) sub = (x > h || y > h);
		else sub = false;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();
	
	x = analogRead(A1);
	y = analogRead(A2);

	disp(num[0], num[0]);
}

void loop() {
	if (add || sub) {
		if (tc >= 500) {
			tc = 0;
			if (add) {
				if (++count > 99) count = 0;
			}
			if (sub) {
				if (--count < 0) count = 99;
			}
		}
	} else tc = 500;

	onedisp(count);
}