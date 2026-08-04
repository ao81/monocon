#include "monocon_chuugoku.h"

const int s1[5][2] = {
	{0,0},
	{0x30,0},
	{0x06,0},
	{0,0x30},
	{0,0x06},
};
const int s2[5][2] = {
	{0,0},
	{1,1},
	{0x40,0x40},
	{8,8},
	{0x80,0x80},
};
int i = 0, j = 0;

Di sw1(d3), sw2(d2), ts(d1);
Enc enc(a3, a4);

void setup() {
	sm._one();
}

void loop() {
	if (sw1.htol() && --i < 1) i = 4;
	if (sw2.htol() && ++i > 4) i = 1;
	int d = enc.delta();
	if (d != 0) {
		j += d;
		if (j < 1) j = 4;
		if (j > 4) j = 1;
	}
	dp(0x00, s1[i][0] | s2[j][0], s1[i][1] | s2[j][1]);

	if (!ts) {
		dm.br();
		sm.br();
	} else {
		static Iv v;
		if (v(5)) sm.cw();
		if (j == 4) dm.ccw(50);
		else if (j == 2) dm.cw(50);
		else dm.br();
	}
}
