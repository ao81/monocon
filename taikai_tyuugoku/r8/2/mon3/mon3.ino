#include "monocon_chuugoku.h"

void setup() {
	begin();
}

Di ts(d1), sw(d3), ph(d4);

void loop() {
	if (!ts) {
		dm.fr();
	} else {
		static Seq q;
		if (q) { // start
			if (sw.htol()) {
				q.next();
			}
		}
		if (q) { // exec
			static const int pow[4] = { 200, 100, 50, 30 };
			static int n;
			if (q.in()) n = 0;
			static Ti t;
			if (ph.ltoh() && n < 4) {
				n++;
				if (n == 4) t.start(1000);
			}
			static int ms[5] = { 100, 150, 200, 500, 800 };
			if (n <= 3) {
				dm.cw(pow[n]);
			} else {
				dm.fr();
				if (t.done()) q.next();
			}
			static Iv v;
			static int rnd = 0;
			if (v(ms[n])) {
				rnd = random(1, 99);
				bz(330, 30);
				dp.n(rnd);
			}
		}
		if (q) {
			bz(523, 500);
			q.to(0);
		}
	}
}
