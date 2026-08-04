#include "monocon_chuugoku.h"

Di ts(d1), sw1(d3), sw2(d2);

void loop() {
	if (ts) {
		int w1 = 0, w2 = 0;
		if (!sw1) w1 = alp[0];
		if (!sw2) w2 = seg[2];
		dp(0x00, w1, w2);
	} else {
		static Seq q;
		if (q) {
			if (q.in()) dp.off();
			if (sw1.htol()) q.next();
		}
		if (q) {
			static int i = 0, dir = 1;
			static Iv t;
			if (q.in()) { i = 0; dir = 1; t.reset(true); }
			static const int ptn[4][2] = {
				{ 0x58, 0x61 },
				{ 0x54, 0x23 },
				{ 0x23, 0x54 },
				{ 0x43, 0x4c },
			};
			if (t(500)) {
				dp(0x00, ptn[i][0], ptn[i][1]);
				i += dir;
				if (i == 3 || i == 0) dir = -dir;
			}
			if (sw2.htol()) q.prev();
		}
	}
}
