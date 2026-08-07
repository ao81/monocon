#include "monocon_chuugoku.h"

Di sw(d1);
Iv v;
int n = 0;

void loop() {
	if (v(1000)) n++;
	n = wrap(n, 0, 999);
	if (sw.htol()) n = 0;
	dp.n(n);
}
