#include "monocon_chuugoku.h"

int presw;
bool swps = false;

void isr() {
	in.fen(p1, p2, ts);
	if (c > 23) c -= 24;
	if (c < 0) c += 24;

	static word i = 0;
	if (i++ > 50) {
		i = 0;
		in.fts(p5);
		in.fsw(p4);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}
}

void setup() {
	begin();
}

void loop() {
	if (swps) {
		swps = false;
		c = 0;
	}

	disp(seg[c / 10], seg[c % 10]);
}
