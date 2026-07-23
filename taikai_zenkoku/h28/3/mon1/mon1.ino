#include "monocon_chuugoku.h"

Di sw(d3), ph(d4), ts(d1);
int cnt = 0;

void loop() {
	if (ph.ltoh()) cnt = (cnt + 1) % 12;
	dp.n(cnt);
	static Sw s;
	if (sw.htol()) s.start();
	if (sw.ltoh()) {
		s.stop();
		if (s < 1000) {
			sm.abso(cnt * 30, ts ? CW : CCW);
		} else {
			sm.abso(0, ts ? CCW : CW);
			cnt = 0;
		}
		s.reset();
	}
	sm.update(2);
}
