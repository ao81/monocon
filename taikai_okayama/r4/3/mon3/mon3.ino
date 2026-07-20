#include "monocon_chuugoku.h"

Enc e(a3, a4);
int i = 0;
const int s[4][2] = {
	{ 0, 0 },
	{ 1, 1 },
	{ 0x40, 0x40 },
	{ 0x08, 0x08 },
};

void loop() {
	int d = e.delta();
	if (d != 0) {
		i += d;
		if (i < 1) i = 3;
		if (i > 3) i = 1;
	}
	dp(0, s[i][0], s[i][1]);
}
