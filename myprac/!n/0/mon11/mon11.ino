#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	if (in.d(d1).level == 1) led(0b010);
	else led(0);
}
