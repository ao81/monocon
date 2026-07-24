#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);

void loop() {
	auto& a = sig(sw1 ^ sw2);
	auto& b = sig(!sw1 && !sw2);
	auto& c = sig(sw1 || sw2);

	static Seq q;
	if (q) {
		if (b) {
			led(B);
			int t = min(b.elapsed() / 100, 20);
			dp(0, seg[t / 10] | SEG_DOT, seg[t % 10]);
			if (b.held(2000, H)) {
				led(G);
				dp.f(2.0);
				bz(1500, 100);
				q.next();
			}
		} else {
			dp.off();
			led(a ? R : 0);
			if (b.htol() && !b.held(2000, H, true)) bz(800, 100);
		}
	}
	if (q) {
		led(G);
		dp.f(2.0);
		if (c) q.prev();
	}
}
