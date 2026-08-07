#include "monocon_chuugoku.h"

Di sw(d1);
Seq q;

void loop() {
	if (q) {
		if (q.in()) bz(1000, 50);
		led(R);
	}
	if (q) {
		if (q.in()) bz(1500, 50);
		led(G);
	}
	if (q) {
		if (q.in()) bz(2000, 50);
		led(B);
	}
	if (sw.htol()) q.next();
}
