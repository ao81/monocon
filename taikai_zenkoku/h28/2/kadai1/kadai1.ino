#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static int cnt = 0;
	if (in.d(d1).ltoh) {
		if (++cnt > 11) cnt = 0;
	}
	dispn(cnt);

	static long push = 0;
	de sw = in.d(d2);
	static ti t;
	if (sw.htol) push = millis();
	if (sw.ltoh) {
		push = millis() - push;
		t.start(1000);
		if (push < 1000) {
			long tgt = lround((long)cnt * SPR / (double)12);
			if (in.d(d3)) { // cw
				sm.seek(tgt, true);
			} else { // ccw
				sm.seek(tgt, false);
			}
		} else { // to 0
			sm.seek(0);
			cnt = 0;
		}
	}
	if (!sm.moving()) {
		sm.off();
	}
	sm.step(2);
}
