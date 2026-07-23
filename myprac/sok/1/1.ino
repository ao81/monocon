#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static iv t;
	if (t(1T00)) {
		int sk = in.sok(a1);
		dispn(sk);
	}
}
