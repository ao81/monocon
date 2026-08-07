#include "monocon_chuugoku.h"

void isr() {}

Timer t;

void setup() {
	begin();
	t.start(3000);
	dispNum(0);
}

void loop() {
	if (t.done()) dispOff();
}
