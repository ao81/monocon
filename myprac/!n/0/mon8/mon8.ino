#include "monocon_chuugoku.h"

const uint8_t s[4] = { 0, 0b001, 0b100, 0b010 };
int i = 0;

void isr() {}

void setup() {
	begin();
}

void loop() {
	if (in.d(d2).fall) {
		if (++i > 3) i = 0;
	}
	led(s[i]);
}
