#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	if (in.d(d2).fall) {
		bz(1000, 50);
	}
}
