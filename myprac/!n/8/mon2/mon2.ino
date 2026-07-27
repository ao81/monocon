#include "monocon_chuugoku.h"

Di sw(d3);
Vr vr(a3, 0, 501);
int n = 0;
uint8_t i = 0;

void loop() {
	n = vr.to(0, 100);
	if (sw.htol()) i = wrap(i + 1, 0, 1);
	if (i == 0) dp.n(n).o(n);
	else dp.base(n, 16, true).o(n);
}
