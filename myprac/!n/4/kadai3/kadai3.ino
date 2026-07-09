#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	if (in.d(d1).held(1000, LOW)) bz(2000, 200);
}
