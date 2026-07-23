#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	Joy j = in.joy(a2, a1);
	int n = j.dir(8);
	if (n == -1) dispOff();
	else dispNum(n);
}
