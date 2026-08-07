// #define dbg
#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2), ts(d1);
Vr vr(a3, 0, 501, 10);
Ti t;
Seq q;

void loop() {
	static int speed, cnt = 0;
	speed = vr.to(0, 255);
	if (q) {
		if (q.in()) dm.br();
		if (sw1.htol()) q.next();
		if (!ts) dp.s("---");
		else dp.n(speed);
		if (q.out()) cnt = 0;
	}
	if (q) {
		if (q.in()) { t.start(2000); cnt++; }
		dm.cw(speed);
		if (t.done()) q.next();
		dp.s("fwd");
	}
	if (q) {
		if (cnt >= 10) q.to(5);
		else {
			if (q.in()) t.start(1000);
			dm.br();
			if (t.done()) q.next();
			dp.s("stp");
		}
	}
	if (q) {
		if (q.in()) { t.start(2000); cnt++; }
		dm.ccw(speed);
		if (t.done()) q.next();
		dp.s("reu");
	}
	if (q) {
		if (cnt >= 10) q.to(5);
		else {
			if (q.in()) t.start(1000);
			dm.br();
			if (t.done()) q.to(1);
			dp.s("stp");
		}
	}
	if (q) {
		dm.br();
		dp.s("end");
		if (sw1.htol()) q.to(0);
	}
	if (sw2.htol()) q.to(0);
	DC(cnt);
}
