#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static Sw s;
	static int n = 0;
	if (in.d(d2).htol) s.start();
	if (in.d(d1).htol) s.stop();
	dispn(s() / 100);
}
