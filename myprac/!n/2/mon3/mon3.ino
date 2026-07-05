#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	int d = in.joy(a1, a2).dir(8);
	if (d == -1) dispOff();
	else dispn((d + 4) % 8);
}
