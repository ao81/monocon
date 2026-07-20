#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);

void loop() {
	if (!sw1 && !sw2) dm.br();
	else if (!sw1) dm.ccw(50);
	else if (!sw2) dm.cw(50);
	else dm.fr();
}
