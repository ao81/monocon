#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	de ts = in.d(d1);
	if (ts.htol || ts.ltoh) {
		led(0);
		dispOff();
	}

	if (ts) {
		if (!in.d(d3)) {
			dm.ccw(50);
		} else if (!in.d(d2)) {
			dm.cw(50);
		} else {
			dm.fr();
		}

		if (in.ref(a2, 950)) {
			led(R);
		} else {
			led(0);
		}

		de ph = in.d(d4);

		static Seq q;
		static Sw s;
		q.top();
		if (q()) {
			if (ph.ltoh) {
				q.next();
				s.start();
			}
			led(G);
		}
		if (q()) {
			if (ph.ltoh) {
				s.stop();
				dispn(s.ms() / 10);
				q.next();
			}
			led(B);
		}
	} else {
		static Seq q;

		if (in.ltoh(d4)) {
			q.next();
		}

		q.top();
		if (q()) {
			static int i = 0;
			if (in.htol(d2)) {
				if (++i > 5) i = 0;
			}
			if (in.htol(d3)) {
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
			n = in.encLoop(a3, a4, -9, 99);
			disp(seg[1], (n < 0 ? SEG_MINUS : (n / 10 == 0 ? 0x00 : seg[abs(n / 10)])), seg[abs(n % 10)], 20, 255, 255);
		}
		if (q()) {
			static iv t;
			if (t(100)) {
				int n = in.sok(a1);
				disp(seg[2], (n / 10 == 0 ? 0x00 : seg[n / 10]), seg[n % 10], 20, 255, 255);
			}
		}
	}
}
