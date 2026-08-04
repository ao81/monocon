#include "monocon_chuugoku.h"

const int s[5][2] = {
	{ 0x00, 0x00 },
	{ 0x30, 0x00 },
	{ 0x06, 0x00 },
	{ 0x00, 0x30 },
	{ 0x00, 0x06 },
};
int i = 0;

Di sw1(d3), sw2(d2);

void loop() {
	if (sw1.htol() && --i < 1) i = 4;
	if (sw2.htol() && ++i > 4) i = 1;
	dp(0x00, s[i][0], s[i][1]);
}
