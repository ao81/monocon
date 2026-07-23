#define USE_TIMER3_ISR
#include "mono_con.h"

const int xseg[3][2] = {
	{ 0x30, 0x00 },
	{ 0x06, 0x30 },
	{ 0x00, 0x06 },
};

const int yseg[3][2] = {
	{ 0x08, 0x08 },
	{ 0x40, 0x40 },
	{ 0x01, 0x01 },
};

int x, y;

word in;

int getidx(int p) {
	return map(p, 0, 1023, 2, -1);
}

void onedisp(int n[2]) {
	static int pren[2] = { 0x00, 0x00 };
	if (memcmp(pren, n, sizeof(pren)) != 0) {
		memcpy(pren, n, sizeof(pren));
		disp(n[0], n[1]);
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
	int xi = getidx(x), yi = getidx(y);
	int seg[2] = {
		xseg[xi][0] | yseg[yi][0],
		xseg[xi][1] | yseg[yi][1],
	};
	onedisp(seg);
}