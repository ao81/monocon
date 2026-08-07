#include "monocon_chuugoku.h"

Di ph(d4), sw(d3), ts(d1);

void loop() {
	static int n = 0;
	if (ph.ltoh()) {
		if (!ts) {
			if (n < 10) {
				n++;
				if (n == 10) bz(880, 150);
			}
		} else {
			if (n < 25) {
				n++;
				if (n == 25) bz(880, 150);
			}
		}
	}
	static Sw s;
	if (sw.held(1000, L, true)) n = 0;
	if (!ts && n > 10) { n = 10; bz(880, 150); }
	if (!ts && n >= 10) led(R);
	else if (ts && n >= 25) led(R);
	else led(G);
	dp.n(n);
}
