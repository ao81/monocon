#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);
Enc enc(a3, a4);
const int s[5][2] = {
	{ 0, 0 },
	{ 0x30, 0 },
	{ 0x06, 0 },
	{ 0, 0x30 },
	{ 0, 0x06 },
};
int i = 0;

void loop() {
	if (sw1.htol() && --i < 1) i = 4;
	if (sw2.htol() && ++i > 4) i = 1;
	int d = enc.delta();
	if (d != 0) {
		i += d;
		if (i < 1) i = 4;
		if (i > 4) i = 1;
	}
	dp(0x00, s[i][0], s[i][1]);
}
