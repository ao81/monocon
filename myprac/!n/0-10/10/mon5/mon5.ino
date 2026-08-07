#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Enc enc(a3, a4);
Seq q;
Sw s;
Ti t;
int light = 100;

void loop() {
	if (q) {
		if (q.in()) s.stop();
		if (sw1.htol()) q.next();
		led(R);
	}
	if (q) {
		if (q.in() && !s.running()) { s.start(); bz(2000, 100); }
		if (sw2.htol()) q.next();
		led(G);
	}
	if (q) {
		if (q.in()) { t.start(2000); bz(1000, 100); }
		if (t.done()) q.prev();
		led(B);
	}
	if (sw3.held(1000, L)) {
		q.toa(0);
		s.reset();
	}
	if (!q.is(2)) {
		if (!ts) {
			dp.f(s.ms() / 10, true);
		} else {
			dp.f(s.s(), true);
		}
	}
	light = enc.clampTo(light, 10, 100);
	dp.per(light);
}
