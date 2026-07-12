#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static Seq q;
	static int ran = 1000;
	static ti a;
	static Sw s;

	if (q(1)) { // wait
		if (in.htol(d3)) {
			ran = rnd(1000, 3000);
			q.next();
		}
		if (a.done()) {
			bzoff();
			dispOff();
		}
	}
	if (q) { // start
		static ti t;
		if (q.in()) t.start(ran);
		if (t.done()) q.next();
		if (in.ltoh(d4)) {
			a.start(1000);
			bz(800);
			disp(0x00, 0x00, seg[0xf]);
			q.to(0);
		}
	}
	if (q) { // time
		if (q.in()) {
			s.start();
		}
		led(G);
		dispn(s);
		if (in.ltoh(d4)) {
			q.next();
		}
	}
	if (q) { // res
		if (in.htol(d2)) {
			q.next();
			led(0);
			dispOff();
		}
	}
}
