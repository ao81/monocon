#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Enc enc(a1, a2);
Pr pr(a3, 930);
Vr vr(a4);

void loop() {
	int color = 0;
	if (!sw1) color |= G;
	if (!sw2) color |= B;
	if (!sw3) color |= R;
	if (!pr) color |= G;
	if (vr.raw() > 450) color |= B;
	if (ts) color |= R;

	static int opac = 100;
	opac = enc.clampTo(opac, 0, 100);

	led(color, opac);
	dp.n(opac);
}
