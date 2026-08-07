#include "monocon_chuugoku.h"

Di sw(d3);

void loop() {
	static uint8_t i = 0;
	if (sw.htol()) i = wrap(i + 1, 0, 3);
	static uint8_t l[4] = { 0, R, G, B };
	led(l[i]);
	dp.n(i);
}
