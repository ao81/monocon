#include "monocon_chuugoku.h"

void setup() {
	begin();
}

int dir = 12;

void loop() {
	if (in.d(d2).htol) {
		if (in.d(d3)) {
			if (++dir > 12) dir = 1;
		} else {
			if (--dir < 1) dir = 12;
		}
		int tgt = sm.divStep(dir);
		sm.seek(tgt);
	}

	if (sm.moving()) led(G);
	else led(0);

	disp(0x00, seg[dir / 10], seg[dir % 10]);
	sm.step(2);
}
