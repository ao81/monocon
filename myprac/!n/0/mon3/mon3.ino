#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	dispDec(ar(a1), 2);
}
