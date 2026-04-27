// 16:14

#define USE_TIMER3_ISR
#include "mono_con.h"

const int l = 0 + 200;
const int h = 1023 - 200;

const int xptn[3][2] = {
	{ 0x30, 0x00 },
	{ 0x06, 0x30 },
	{ 0x00, 0x06 }
};

const int yptn[3][2] = {
	{ 0x08, 0x08 },
	{ 0x40, 0x40 },
	{ 0x01, 0x01 }
};

int x, y;

word in;

void onedisp(int p[2]) {
	static int prep[2] = { 0x00, 0x00 };
	if (memcmp(p, prep, sizeof(prep)) != 0) {
		memcpy(prep, p, sizeof(prep));
		disp(p[0], p[1]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		y = analogRead(A2);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	y = analogRead(A2);
}

void loop() {
	int ptn[2] = { 0x00, 0x00 };

	if (x > h) {
		ptn[0] |= xptn[0][0];
		ptn[1] |= xptn[0][1];
	} else if (x < l) {
		ptn[0] |= xptn[2][0];
		ptn[1] |= xptn[2][1];
	} else {
		ptn[0] |= xptn[1][0];
		ptn[1] |= xptn[1][1];
	}

	if (y > h) {
		ptn[0] |= yptn[0][0];
		ptn[1] |= yptn[0][1];
	} else if (y < l) {
		ptn[0] |= yptn[2][0];
		ptn[1] |= yptn[2][1];
	} else {
		ptn[0] |= yptn[1][0];
		ptn[1] |= yptn[1][1];
	}

	onedisp(ptn);
}