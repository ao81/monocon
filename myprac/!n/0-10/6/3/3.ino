#include "monocon_chuugoku.h"

void setup() {
	begin();
}

Di sw1(d3), sw2(d2), ph(d4), ts(d1);
Pr pr(a2);
Vr vr(a3);

int soku = 0;

void loop() {
	if (!ts) {
		soku = 0;
		dp.s("off");
		led(0);
		bz.off();
		return;
	}


}
