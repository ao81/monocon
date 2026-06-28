#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	if (in.d(d3).level == 1) led(0b100);
	else led(0b001);
}
