#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	int n = in.encClamp(a1, a2, 0, 255);
	int m = map(n, 0, 255, 0, 100);
	dispn(m);
	led(0b111, n);
}
