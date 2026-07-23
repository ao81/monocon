#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	int n = in.enc(a1, a2);
	dispNum(clampv(n, 0, 100));
}
