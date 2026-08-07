#include "monocon_chuugoku.h"

word w = 0;
void isr() { w++; }

void setup() {
	begin();
}

void loop() {
	int tsw = in.d(d3).level;
	if (w >= 20) {
		w = 0;
		if (tsw == 1) spm.cw();
		else spm.ccw();
	}
}
