#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);
Sig r;

void loop() {
	r(!sw1 && !sw2);
	if (r) led(G);
	else led();
}
