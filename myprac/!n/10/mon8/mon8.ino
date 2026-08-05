#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Js js(a1, a2, 2);
Enc enc(a3, a4);
Seq q;

int speed = 0;
bool move = false;
bool isbr = true;

void loop() {
	speed = enc.clampTo(speed, 0, 255);
	dp.n(speed);

	if (!ts) {
		dm.fr();
		move = false;
		q.toa(0);
	} else {
		if (q) {
			if (sw1.htol()) move = true;
			if (move) {
				int d = js.dir(4);
				if (d == 0) { dm.cw(speed); led(G); }
				else if (d == 2) { dm.ccw(speed); led(B); }
				else {
					if (isbr) dm.br();
					else dm.fr();
					led(Y);
				}
				if (sig(d).change() && (d == 0 || d == 2)) {
					bz(1000, 100);
				}
				if (sw2.htol()) isbr = !isbr;
				if (sw3.htol()) q.next();
			}
		}
		if (q) {
			dm.br();
			static Iv v;
			if (v(500)) {
				led(tog() ? R : 0);
			}
		}
	}
}
