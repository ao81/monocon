#include "monocon_chuugoku.h"

Di sw(d1);
Ti t;
int n = 0;

void loop() {
	if (sw.htol()) t.start(3000);
	dp.n((t.remain() + 999) / 1000);
	if (t.active()) led(R);
	if (t.done()) { led(G); bz(2000, 100); }
}
