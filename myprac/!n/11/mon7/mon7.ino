#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2);
Ti t;
int n = 0;

void loop() {
	if (sw1.htol()) n++;
	n = wrap(n, 0, 999);
	if (sw2.held(1000, L)) { n = 0; t.start(100); led(G); }
	if (t.done()) led();
	dp.n(n);
}
