#include "monocon_chuugoku.h"

Di sw(d3);

void loop() {
	static Sw s;
	static Ti ti;
	if (sw.htol()) { s.start(); }
	if (sw.ltoh()) {
		static Tog t;
		if (s.ms() < 1000) {
			if (t()) led(G);
			else led();
		} else if (s.ms() < 3000) {
			led(B);
			dp.f(s.s());
			ti.start(2000);
		} else {
			bz(800, 200);
			dp.f(s.s());
			ti.start(2000);
		}
		s.reset();
	}
	if (ti.done()) dp.off();
}
