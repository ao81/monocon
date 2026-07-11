#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static int mode = 0;
	static Seq q;
	if (q(true)) {
		mode = in.encLoop(a3, a4, 0, 2);
		led(0);
		if (in.htol(d3)) q.next();
	}
	if (q) {
		static const int l[3] = { R, G, B };
		led(l[mode]);
		if (in.htol(d2)) q.next();
	}
	dispn(mode);
}
