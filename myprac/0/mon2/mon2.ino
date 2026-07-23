#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	int n = map(ar(a1), 0, 1023, 0, 100);
	dispNum(n);
}
