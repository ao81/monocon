#include "monocon_chuugoku.h"

void setup() {
	begin();
}

Seq q;
iv t;

void loop() {
	de ph = in.d(d1);
	de sw = in.d(d2);
	de ts = in.d(d3);

	if (sw.htol) q.next();
	if (ph.ltoh) q.prev();

	q.top();
	if (q()) {
		led(G);
		dispn(12.3);
	}
	if (q()) {
		led(B);
	}
	if (q()) {
		static const int n[] = { nc4, nc5, nr, nf5, na5, ne5, nb4 };
		static const int d[] = { 100, 300, 100, 100, 100, 100, 200 };

		if (q.in()) {
			mel.play(n, d, 7);
		}

		dispn(1.23);
		led(R);
	}
	if (q()) {
		led(0);
		dispn(123);
	}

	mel.update();
}
