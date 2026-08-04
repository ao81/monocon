#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Enc enc(a3, a4);
Seq q;
Ti t;
Sw s;

int cnt = 1;
int wait = 2000;

void loop() {
	if (q) { // 回数選択
		if (q.in()) cnt = 1;
		cnt = enc.clampTo(cnt, 1, 5);
		dp.n(cnt);
		if (sw1.htol()) q.next();
		led();
	}
	if (q) { // 待ち
		if (q.in()) {
			if (!ts) wait = random(2000, 4001);
			else wait = random(1000, 6001);
			t.start(wait);
		}
		led();
		dp.off();
		if (sw2.htol()) q.tor(2);
		if (t.done()) q.next();
	}
	if (q) { // 計測開始
		if (q.in()) { bz(1500, 50); s.reset(); s.start(); }
		led(G);
		if (sw2.htol()) q.tor(2);
		if (q.out()) s.stop();
	}
	if (q) {
		if (q.in()) { bz(800, 1000); s.reset(); s.start(); }
		led(R);
		if (sw2.htol()) q.next();
		if (q.out()) s.stop();
	}
	if (q) {
		if (q.in()) { t.start(2000); }
		dp.f(s.ms() / 10);
		if (t.done()) {
			if (--cnt > 0) {
				q.tor(-3);
			} else {
				q.toa(0);
			}
		}
	}
	if (sw3.htol()) q.toa(0);
}
