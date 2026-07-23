#include "monocon_chuugoku.h"

void setup() {
	begin();
	sm._one();
}

Seq q;
di sw1(d3), sw2(d2), ph(d4);
enc en(a3, a4);

void loop() {
	if (ph.ltoh()) {
		q.next();
	}

	if (q) {
		led(0);
		if (sw1.htol()) sm.rela(-30);
		else if (sw2.htol()) sm.rela(30);
		dp.n(sm.pos());
		led(R);
	}
	if (q) {
		static int n = 0;
		n += en.delta();
		if (n < 0) n = 11;
		if (n > 11) n = 0;
		dp.n(n);
		if (sw1.htol()) sm.abso(n * 30, true);
		led(G);
	}

	sm.update(2000);
}
