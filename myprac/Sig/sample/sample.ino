#include "monocon_chuugoku.h"

Pr pr(a2);
Sig si(10);

void loop() {
	si.set(pr.raw() <= 950);
	if (si.ltoh()) bz(2000, 100);
}
