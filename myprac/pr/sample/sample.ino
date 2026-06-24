#include "monocon_chuugoku.h"

const int s[4][2] = {
	{ 0x30, 0x00 },
	{ 0x06, 0x00 },
	{ 0x00, 0x30 },
	{ 0x00, 0x06 },
};

int idx = 0;
bool f = true;

void isr() {
	static word i = 0;
	if (i++ >= 1000) {
		i = 0;
		f = true;
	}
}

void setup() {
	begin();
}

void loop() {
	if (f) {
		f = false;
		in.fpr(p1);
		p /= 10;
		if (p < 25) idx = 0;
		else if (p < 50) idx = 1;
		else if (p < 75) idx = 2;
		else idx = 3;
		disp(s[idx][0], s[idx][1]);
	}
}
