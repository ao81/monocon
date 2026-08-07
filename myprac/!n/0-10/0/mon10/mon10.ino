#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	int n = in.enc(a1, a2);
	if (n < 0) in.encSet(999);
	if (n > 999) in.encSet(0);
	if (n < 300) led(0b010);
	else if (n <= 700) led(0b100);
	else led(0b001);
	dispNum(n);
}
