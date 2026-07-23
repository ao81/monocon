#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	de ts = in.d(d0);
	if (ts.htol || ts.ltoh) {
		led(0);
		dispOff();
	}

	if (ts) {
		if (!in.d(d2)) {
			dm.ccw(50);
		} else if (!in.d(d1)) {
			dm.cw(50);
		} else {
			dm.fr();
		}

		if (in.ref(a1, 950)) {
			led(R);
		} else {
			led(0);
		}

		de ph = in.d(d3);

		static Seq q;
		q.top();
		if (q()) {
			if (ph.ltoh) {
				q.next();
			}
			led(G);
		}
		if (q()) {
			static int cnt = 0;
			static int pre = 0;
			de dp = in.d(PH);
			if (dp.ltoh) {
				if (dm.now == 1) {
					if (++cnt > 99) cnt = 0;
					pre = 1;
				} else if (dm.now == -1) {
					if (--cnt < -99) cnt = 99;
					pre = -1;
				} else {
					if (pre == 1) {
						if (++cnt > 99) cnt = 0;
					} else if (pre == -1) {
						if (--cnt < -99) cnt = 0;
					}
				}
			}
			dispn(cnt);

			if (ph.ltoh) {
				q.next();
			}
			led(B);
		}
	} else {
		static Seq q;

		if (in.ltoh(d3)) {
			q.next();
		}

		q.top();
		if (q()) {
			static int i = 0;
			if (in.htol(d1)) {
				if (++i > 5) i = 0;
			}
			if (in.htol(d2)) {
				if (--i < 0) i = 5;
			}
			led(R, i * 20);
			disp(seg[0], 0x00, seg[i], 20, 255, 255);
		}
		if (q()) {
			if (q.in()) {
				led(0);
			}
			static int n = 0;
			n = in.encLoop(a2, a3, -9, 99);
			disp(seg[1], (n < 0 ? SEG_MINUS : (n / 10 == 0 ? 0x00 : seg[abs(n / 10)])), seg[abs(n % 10)], 20, 255, 255);
		}
		if (q()) {
			static iv t;
			if (t(100)) {
				int n = in.sok(a0);
				disp(seg[2], (n / 10 == 0 ? 0x00 : seg[n / 10]), seg[n % 10], 20, 255, 255);
			}
		}
	}
}
