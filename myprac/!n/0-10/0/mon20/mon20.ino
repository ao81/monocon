#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
	bz(2000, 30);
}

Every e;

void loop() {
	if (e(1000)) {
		bz(2000, 30);
	}
}
