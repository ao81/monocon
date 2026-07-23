#include "monocon_chuugoku.h"

Enc enc(a3, a4);
int n = 0;

void loop() {
	n = enc.loopTo(n, 0, 2);
	dp.n(n);
	static Seq q;
	q.to(n);
	if (q) {
		led(R);
	}
	if (q) {
		led(B);
	}
	if (q) {
		led(G);
	}
}
