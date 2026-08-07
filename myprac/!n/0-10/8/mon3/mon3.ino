#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);
Sw s;
Tog t;

void loop() {
	if (sw1.htol()) {
		if (t()) s.start();
		else s.stop();
	}
	if (sw2.htol()) s.reset();
	if (s.s() <= 999) dp.f(s.s(), true);
	else dp.s("end");
	led(s.running() ? G : R);
}
