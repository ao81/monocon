#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

Every t;

void loop() {
	static bool s = true;
	if (t(250)) {
		s = !s;
	}
	if (s) led(1);
	else led(0);
}
