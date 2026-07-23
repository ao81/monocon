#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

const int d[4] = { 0, 0b001, 0b100, 0b010 };

void loop() {
	int n = in.enc(a1, a2);
	int i = n % 4;
	led(d[i]);
}
