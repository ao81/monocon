#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static iv t;
	if (t(100)) {
		int cm = in.sok(a1);
		dispn(cm);
		if (cm <= 9) led(R);
		else if (cm >= 26) led(G);
		else led(B);
		if (cm < 5) bz(2000);
		else bzoff();
	}
}
