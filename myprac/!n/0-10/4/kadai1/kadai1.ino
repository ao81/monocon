#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	if (in.d(d0)) led(G);
	else led(0);
}
