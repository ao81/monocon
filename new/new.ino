#include "monocon_chuugoku.h"

void setup() {
	begin();
}

di sw1(d3), sw2(d2), ts(d1), ph(d4);
enc en(a3, a4);
pr r(a2);

void loop() {
	static Seq q;
	static int n = 1;
	static int now = 0;

	if (ts == L) q.to(0);

	if (q) {
		n += en.delta();
		if (n < 1) n = 1;
		if (n > 99) n = 99;
		dp.n(n);
		led(B);
		dm.fr();
		if (ts == H) q.to(1);
		now = 0;
	}
	if (q) {
		if (q.in()) now = 0;
		led(R);
		dm.fr();
		dp.off();
		if (sw1.htol()) q.next();
	}
	if (q) {
		dm.cw(100);
		led(G);
		if (ph.ltoh()) {
			if (r == H) now++;
			else {
				bz(800, 100);
				led(R);
			}
		}
		dp.n(now);
		if (now >= n) {
			q.next();
		}
	}
	if (q) {
		if (q.in()) {
			dm.br();
			led(R);
			bz(1500, 300);
			dp.s("end");
		}
		if (sw2.htol()) {
			now = 0;
			dm.br();
			bz.off();
			q.to(0);
		}
	}
}
