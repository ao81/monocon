#include "monocon_chuugoku.h"

void setup() {
	begin();
}

word tc = 0;
void isr() {
	tc++;
}

void loop() {
	de btn = in.d(d2);

	int c = in.enc(a1, a2);
	static int prec = -1;

	if (c != prec) {
		prec = c;
		spm.mv(c * 4);
	}

	if (tc >= 20) {
		tc = 0;
		spm.run();
	}
}
