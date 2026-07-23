#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	if (in.d(d3).fall) {
		bz(800, 1000);
	}
}
