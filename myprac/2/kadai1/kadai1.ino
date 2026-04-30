#define USE_TIMER3_ISR
#include "mono_con.h"

int x, y;
int xidx, yidx;

word in;

int postoidx(int p) {
	return map(p, 0, 1023, 9, 0);
}

void onedisp(int n, int m) {
	static int pren = -1, prem = -1;
	if (pren != n || prem != m) {
		pren = n, prem = m;
		disp(num[n], num[m]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		y = analogRead(A2);

		xidx = postoidx(x);
		yidx = postoidx(y);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	y = analogRead(A2);

	xidx = postoidx(x);
	yidx = postoidx(y);
}

void loop() {
	onedisp(xidx, yidx);
}