#include "monocon_chuugoku.h"

Di ph(d4), sw(d3), ts(d1);

void loop() {
	ST Seq q;
	if (q) {
		ST Ti t;
		ST Tog g;
		ST Iv v;
		if (q.in()) t.start(3000);
		if (v(100)) {
			if (g()) dp.s("---");
			else dp.off();
		}
		if (t.done()) q.next();
	}
	ST int n = 0;
	if (q) {
		ST Iv v;
		if (q.in()) {
			n = 0;
			v.reset(true);
		}
		if (v(1000)) {
			if (++n > 6) n = 1;
			dp.n(n, true);
		}
		if (sw.htol()) q.next();
	}
	if (q) {
		ST Ti t;
		ST Tog g;
		ST Iv v;
		if (q.in()) t.start(2000);
		if (v(100)) {
			if (g()) dp.n(n, true);
			else dp.off();
		}
		if (t.done()) q.to(n + 2);
		if (q.out()) dp.off();
	}
	if (q) { // 1
		if (ts) {
			dp.s("__H");
			if (!sw) dm.cw(50);
			else dm.fr();
		} else {
			dp.s("__L");
			if (!sw) dm.ccw(100);
			else dm.fr();
		}
		if (ph.htol()) q.to(0);
	}
	if (q) { // 2
		if (ts) {
			dp.s("__H");
			if (sw.htol()) sm.rela(-45);
		} else {
			dp.s("__L");
			if (sw.htol()) sm.rela(45);
		}
		if (ph.htol()) q.to(0);
	}
	if (q) { // 3
		ST const uint8_t l[8] = { 0, R, G, B, M, Y, C, W };
		ST Iv v;
		ST int i = 0;
		if (q.in()) {
			i = 0;
			v.reset(true);
		}
		if (v(1000)) {
			led(l[i]);
			i = (i + 1) % 8;
		}
		if (ph.htol()) q.to(0);
		if (q.out()) led(0);
	}
	if (q) { // 4
		static Sok sok(a1);
		static auto sample10 = [](int value) {
			return ((value + 9) / 10) * 10;
		};
		dp.n(sample10(sok.cm()));
		if (ph.htol()) q.to(0);
	}
	if (q) { // 5
		static int i = 0;
		if (q.in()) {
			sm.abso(0);
			i = 0;
		}
		if (ts) {
			if (sw.htol()) i = (i + 1) % 8;
		} else {
			if (sw.htol()) sm.abso(45 * i, SHORT);
		}
		dp.n(i);
		if (ph.htol()) q.to(0);
	}
	if (q) { // 6 (g1)
		/*static Sok sok(a1);
		dm.cw(80);*/
		if (ph.htol()) q.to(0);
	}
	sm.update(2);
	if ((!q.is(4) || !q.is(7)) && !sm.busy()) sm.fr();
}
