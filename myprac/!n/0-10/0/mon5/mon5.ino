#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

Every t;

void loop() {
	static int n = 0;
	if (t(500)) {
		if (++n > 9) n = 0;
	}
	dispNum(n);
}
