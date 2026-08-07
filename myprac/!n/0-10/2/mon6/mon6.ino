#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static int go = 0;
	de ts = in.d(d3);
	de sw = in.d(d2);

	if (sw.htol) {
		if (ts) go = (go + 360 + 90) % 360;
		else go = (go + 360 - 90) % 360;
	}

	sm.seekDeg(go);
	sm.step(2);

	dispn(go);
}
