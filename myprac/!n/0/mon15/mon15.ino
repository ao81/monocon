#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	int n = in.enc(a1, a2);
	static int pren = -1;
	if (in.d(d2).level == 0) {
		if (n != pren) {
			pren = n;
			bz(n * 100 + 100);
		}
	} else {
		pren = -1;
		bzoff();
	}
}
