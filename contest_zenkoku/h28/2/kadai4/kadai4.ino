#include "monocon_chuugoku.h"

int cnt = 30;
iv t;

void setup() {
	begin();
	dispn(cnt);
	t.wait();
}

void loop() {
	de ts = in.d(d3);
	if (ts.ltoh) t.go();
	if (ts) {
		if (t(1000)) {
			if (cnt > 0) cnt--;
		}
	} else {
		if (in.d(d2).htol) {
			int tgt = sm.divStep(cnt % 10);
			sm.seek(tgt);
		}
	}
	if (ts.htol) t.wait();
	dispn(cnt);
	sm.step(2);
}
