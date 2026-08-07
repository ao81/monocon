#include "monocon_chuugoku.h"

Enc enc(a1, a2);
int n = 50;

void loop() {
	n = enc.clampTo(n, 0, 100);
	dp.n(n);
}
