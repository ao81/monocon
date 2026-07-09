#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	if (in.ref(a1, 930)) led(R);
	else led(GBR);
}
