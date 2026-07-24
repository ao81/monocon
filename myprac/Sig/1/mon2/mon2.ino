#include "monocon_chuugoku.h"

Enc enc(a3, a4);
Sig g;
int n = 0;

void loop() {
	n = enc.loopTo(n, 0, 100);
	g(n >= 48 && n <= 52);
	if (g) led(G);
	else led();
	if (g.ltoh()) bz(2000, 100);
	if (g.htol()) bz(800, 100);
	dp.n(n);
}
