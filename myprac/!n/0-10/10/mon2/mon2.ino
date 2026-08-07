#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Enc enc(a3, a4);
int n = 0;

void loop() {
	n += enc.delta();
	if (sw1.htol()) n = 0;
	if (sw2.htol()) n += 10;
	if (sw3.htol()) n -= 10;
	if (!ts) n = clamp(n, 0, 99);
	else n = wrap(n, 0, 99);
	if (sig(n, 0).change()) bz(1000, 30);
	if (n == 0) led(R);
	else if (n == 99) led(G);
	else led(B);
	dp.n(n);
}
