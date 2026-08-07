#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2);
Seq q;

void loop() {
	if (q) {
		led();
		dp.n(0);
		if (sw1.htol()) q.next();
	}
	if (q) {
		led(R);
		if (q.after(2000)) q.next();
	}
	static Sw s;
	if (q) {
		if (q.in()) { s.reset(); s.start(); }
		led(G);
		if (sw2.htol()) { s.stop(); q.next(); }
	}
	if (q) {
		dp.f(s.s());
		if (q.after(2000)) q.toa(0);
	}
}
