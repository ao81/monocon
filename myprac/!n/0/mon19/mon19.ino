#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	static int n = 0;
	if (in.d(d2).fall) {
		if (++n > 99) n = 0;
	}
	dispNum(n);
}
