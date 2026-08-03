#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Enc enc(a3, a4);
Seq q;
Iv v;
int time = 10, idx = 0;

const int ptn3[3] = { R, G, B };
const int ptn7[7] = { R, GR, G, GB, B, BR, W };

void loop() {
	if (q) {
		if (q.in()) { led.per(10); enc.delta(); }
		time += enc.delta() * 5;
		time = clamp(time, 10, 100);
		dp.n(time);
	}
	if (q) {
		if (q.in()) { idx = 0; bz(2000, 30); led.per(100); }
		if (v(time * 10)) {
			led(!ts ? ptn3[idx] : ptn7[idx]);
			idx = wrap(idx + 1, 0, (!ts ? 2 : 6));
			bz(2000, 30);
		}
	}
	if (sw1.htol()) q.next();
}
