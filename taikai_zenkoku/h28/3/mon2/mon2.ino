#include "monocon_chuugoku.h"

Di sw(d3), ts(d1);

void loop() {
	static int n = 12;
	if (sw.htol()) {
		if (ts) {
			if (--n < 1) n = 12;
		} else {
			if (++n > 12) n = 1;
		}
		sm.abso(n * 30);
	}
	sm.update(2);
	dp.n(n);
	if (sm.busy()) led(G);
	else led(0);
}
