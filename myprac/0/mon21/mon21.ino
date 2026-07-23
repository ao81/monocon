#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	dispNum(in.enc(a1, a2));
}
