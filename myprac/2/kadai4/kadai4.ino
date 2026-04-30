#define USE_TIMER3_ISR
#include "mono_con.h"

int x, y;
int cnt = 0;

word in, tc;

int st = 1;

int getst(int x, int y) {
	int p = abs(2*x - 1023) > abs(2*y - 1023) ? x : y;
	return map(p, 0, 1023, 0, 3);
}

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
	st = getst(x, y);

	if (st == 0) {
		if (tc >= 500) {
			tc = 0;
			if (++cnt > 99) cnt = 0;
		}
	} else if (st == 2) {
		if (tc >= 500) {
			tc = 0;
			if (--cnt < 0) cnt = 99;
		}
	} else {
		tc = 500;
	}

	onedisp(cnt);
}