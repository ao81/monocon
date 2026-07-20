#include "monocon_chuugoku.h"

Di ph(d4), sw1(d3);

void loop() {
	static Seq q;
	static int cnt = 0;
	static Sw s;
	if (sw1.htol()) q.to(0);
	if (q) {
		if (q.in()) { s.reset(); dp.off(); }
		if (ph.ltoh()) { cnt = 1; q.next(); s.start(); }
	}
	if (q) {
		if (ph.ltoh()) cnt++;
		dp.n(cnt);
		if (cnt >= 20) q.next();
	}
	if (q) {
		if (q.in()) {
			s.stop();
		}
		dp.f(clamp(s / 1000.0f, 0.0f, 99.9f));
	}
}
