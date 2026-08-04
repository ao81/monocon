#include "monocon_chuugoku.h"

Di ts(d1), sw1(d3), sw2(d2);

void loop() {
	static Seq q;
	if (ts.htol()) q.to(0);
	if (q) {
		if (q.in()) dp.off();
		if (ts) q.next();
	}
	if (q) {
		static Ti t;
		if (q.in()) t.start(3000);
		if (t.done()) q.next();
		dp.n((t.remain() + 999) / 1000);
	}
	if (q) {
		if (q.in()) {
			sm.zero();
			sm.rela(90);
		}
		sm.update(5);
		if (!sm.busy() || sw1.htol()) q.next();
	}
	if (q) {
		static int n, cnt;
		static bool ok, tgl;
		if (q.in()) { n = sm.remainDeg(); ok = false; cnt = 6; tgl = true; }
		if (n >= 36 && n <= 54) ok = true;
		static Iv t;
		if (t(250)) {
			if (cnt > 0) {
				if (tgl) {
					if (ok) dp(0x00, 0x7f, 0x7f);
					else dp(0x00, 0x49, 0x49);
				}
				else dp.off();
				tgl = !tgl;
				cnt--;
			} else {
				dp.off();
				q.next();
			}
		}
	}
	if (q) {}
}
