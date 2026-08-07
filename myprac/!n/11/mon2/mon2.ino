#include "monocon_chuugoku.h"

Di sw(d1);
const int l[3] = { R, G, B };
int i = 0;

void loop() {
	if (sw.htol()) i = wrap(i + 1, 0, 2);
	led(l[i]);
}
