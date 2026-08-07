#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static int p = 0;
	de ph = in.d(PH);
	if (ph.ltoh) p++;
	if (p < 20) {
		dm.drive(50);
	} else dm.fr();
	dispn(p);
	if (in.d(d1).htol) p = 0;
}
