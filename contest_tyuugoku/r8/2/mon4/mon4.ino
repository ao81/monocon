#include "monocon_chuugoku.h"

void setup() {
	begin();
}

Di ts(d1), sw(d3), ph(d4);
Js js(a3, a4);
int data[512] = { 0 };

int play() {
	int i = js.dir(8, 2);
	static const int color[9] = { 0, R, BR, G, GBR, B, GB, G, GR };
	led(color[i + 1]);
	int x = map(js.x, 0, 1023, 50, -50);
	int y = map(js.y, 0, 1023, 1500, 100);
	if (sw) {
		bz(x + y);
		return x + y;
	} else {
		bz.off();
		return 0;
	}
}

void loop() {
	if (!ts) {
		play();
		dp.off();
	} else {
		static Seq q;
		if (q) {
			if (sw.htol()) q.next();
		}
		if (q) {
			static Ti t;
			if (q.in()) t.start(1000);
			if (t.done()) q.next();
		}
		if (q) {
			static Ti t;
			static int idx = 0;
			if (q.in()) {
				t.start(5000);
				idx = 0;
			}
			static Iv v;
			if (v(10)) {
				data[idx++] = play();
			}
			if (t.done() || idx >= 500) q.next();
		}
		if (q) {
			bz.off();
			if (sw.htol()) {
				q.next();
			}
			if (ph.ltoh()) {
				for (int i = 0; i < 512; i++) data[i] = 0;
				q.to(0);
			}
		}
		if (q) {
			static int idx = 0;
			if (q.in()) idx = 0;
			static Iv t;
			if (t(10)) {
				bz(data[idx++]);
			}
			if (idx >= 500 || sw.htol()) q.prev();
		}
		dp.n(q.now());
	}
}
