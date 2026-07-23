#include "monocon_chuugoku.h"

Di ts(d1), sw1(d3), sw2(d2);

void loop() {
	if (!ts) {
		int l = 0;
		if (!sw1) l |= R;
		if (!sw2) l |= G;
		if (!sw1 && !sw2) l |= B;
		led(l);
	} else {
		static Seq q;
		if (q) {
			if (sw1.htol()) q.next();
		}
		if (q) {
			static const int ptn[6] = { R, GR, GBR, GB, B, 0 };
			static int j, cnt;
			static Iv t;
			if (q.in()) { j = 0; cnt = 5; t.reset(true); }
			if (t(200)) {
				led(ptn[j]);
				if (++j >= 6) {
					j = 0;
					if (--cnt <= 0) q.prev();
				}
			}
		}
	}
}
