#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	int n = in.enc(a1, a2);
	if (n < -10) {
		n = -10;
		in.encset(-10);
	} else if (n > 10) {
		n = 10;
		in.encset(10);
	}

	int m = map(n, -10, 10, -255, 255);
	dcm.drive(m);

	dispNum(n);
}
