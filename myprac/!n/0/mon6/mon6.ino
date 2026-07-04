#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	if (in.d(d3).level) {
		dispNum(888);
	} else dispOff();
}
