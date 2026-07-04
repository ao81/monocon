#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	int e = in.enc(a1, a2);
	static int pre = e;
	if (pre != e) {
		pre = e;
		bz(1000, 20);
	}
}
