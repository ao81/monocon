#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	if (in.d(d2).level == 0) {
		led(0b001);
	} else led(0);
}
