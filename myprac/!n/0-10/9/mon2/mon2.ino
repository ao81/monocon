#include "monocon_chuugoku.h"

Di sw(d3), ph(d4);
Pr pr(a2);
const bool ans[3] = { 1, 0, 1 };
int i = 0;
Seq q;

void loop() {
	if (q) {
		int n = ph.ltoh() ? 1 : pr.htol() ? 0 : -1;
		if (n != -1 && q.after(100)) {
			if (ans[i] == n) {
				if (++i > 2) q.to(1);
				else q.restart();
			} else q.to(2);
		}
		if (i != 0 & q.after(3000)) {
			q.to(2);
		}
		dp.n(i);
	}
	if (q) {
		if (q.in()) { bz(2000, 500); led(G); }
		if (q.after(2000)) { q.to(0); i = 0; led(); }
	}
	if (q) {
		if (q.in()) { bz(800, 500); led(R); };
		if (q.after(2000)) { q.to(0); i = 0; led(); }
	}
	if (sw.htol()) {
		i = 0;
		q.to(0);
		led();
	}
}
