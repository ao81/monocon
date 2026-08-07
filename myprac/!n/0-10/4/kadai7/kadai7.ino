#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	de ph = in.d(d3);
	if (ph) led(B);
	else led(0);
	static int cnt = 0;
	if (ph.ltoh) cnt = (cnt + 1) % 999;
	dispn(cnt);
}
