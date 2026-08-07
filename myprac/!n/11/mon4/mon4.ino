#include "monocon_chuugoku.h"

Enc enc(a1, a2);
int i = 50;

void loop() {
	i += enc.delta() * 10;
	i = clamp(i, 0, 100);
	led(G).per(i);
	dp.n(i);
}
