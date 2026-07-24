#include "monocon_chuugoku.h"

Enc enc(a3, a4);
int n = 0;

void loop() {
	n = enc.clampTo(n, 0, 100);
	auto& g = sig(n >= 40 && n <= 60);
	if (g) led(G);
	else led(R);
	if (g.ltoh()) bz(1500, 100);
	if (g.htol()) bz(500, 100);
	dp.n(n);
}
