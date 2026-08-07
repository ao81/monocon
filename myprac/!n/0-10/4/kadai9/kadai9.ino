#include "monocon_chuugoku.h"

void setup() {
	begin();
}

int n = 0;

void loop() {
	if (in.d(d0)) {
		n = in.encClamp(a2, a3, 0, 255);
		dm.drive(n);
		dispn(n);
	} else {
		dm.fr();
		dispOff();
		n = 50;
	}
}
