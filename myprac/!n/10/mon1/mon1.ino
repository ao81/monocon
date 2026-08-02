#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);

void loop() {
	led(!sw2, !sw3, !sw1).per(ts ? 30 : 100);
	dp.n(!sw1 * 100 + !sw2 * 10 + !sw3, true);
}
