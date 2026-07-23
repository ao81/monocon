#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	static const int spd = 80;
	if (in.d(d2).level == 0) dcm.cw(spd);
	else dcm.br();
}
