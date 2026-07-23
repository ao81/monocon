#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	de ts = in.d(d3);
	de sw = in.d(d2);
	de ph = in.d(d1);

	static bool move = false;
	static bool dir = true;
	static ti t;
	static bool waiting = false;

	if (sw.htol) {
		move = !move;
		if (move) {
			dir = !dir;
			int tgt = sm.divStep(dir ? 3 : 9);
			sm.seek(tgt, dir);
			waiting = false;
		}
	}

	if (move) {
		if (!sm.moving() && !waiting) {
			t.start(1000);
			waiting = true;
		}
		if (waiting && t.done()) {
			dir = !dir;
			int tgt = sm.divStep(dir ? 3 : 9);
			sm.seek(tgt, dir);
			waiting = false;
		}
	}

	sm.step(2);
}
