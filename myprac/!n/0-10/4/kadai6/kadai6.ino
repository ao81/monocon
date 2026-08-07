#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static iv t;
	if (t(100)) {
		int n = in.sok(a0);
		dispn(n);
		if (n < 10) led(R);
		else led(0);
	}
}
