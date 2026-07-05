#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	de ph = in.d(d1);
	de sw = in.d(d2);
	de ts = in.d(d3);
	static int n = 0;

	if (ph.ltoh && n < 999) n++;
	if (sw.htol && n > -99) n--;
	if (ts.ltoh) n = 0;

	dispn(n);
}
