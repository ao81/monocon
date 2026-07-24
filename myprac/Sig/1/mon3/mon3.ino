#include "monocon_chuugoku.h"

Di sw(d3);
Sig g;

void loop() {
	g(!sw);
	static bool ok = false;
	if (g.held(2000, H)) { bz(2000, 200); ok = true; }
	if (g == L) ok = false;
	if (ok) led(G);
	else if (g == H) led(B);
	else led();
}
