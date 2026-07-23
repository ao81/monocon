#include "monocon_chuugoku.h"

int w = 0;
void isr() { w++; }

void setup() {
	begin();
}

void loop() {
	if (in.d(d2).fall) {
		spm.mv(2048);
	}

	if (w > 15) {
		w = 0;
		spm.run();
	}
}
