#include "monocon_chuugoku.h"

void setup() {
	begin();
	sm._one();
}

Di ts(d1), sw(d3), ph(d4);

void loop() {
	static bool force = false;
	if (ph) force = true;
	if (!force) {
		if (!ts) {
			led(B);
			dp.n(0);
			bz.off();
		} else {
			led(GR);
			static int n = 0;
			if (sw.htol() || sw.ltoh()) {
				sm.rela(45);
				n = (n + 1) % 8;
			}
			dp.n(n);
			sm.update(2);
		}
	} else {
		led(R);
		bz(2000);
		if (!ts) {
			force = false;
			bz.off();
		}
	}
}
