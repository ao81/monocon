#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);
Enc enc(a3, a4);
int n = 0;

void loop() {
	n = enc.clampTo(n, 0, 999);
	dp.n(n, true);
	if (sw1.held(1000, L)) led(G);
	static int pre = n;
	if (pre != n) { pre = n; led(); }
	if (sw2.htol()) n = 0;
}
