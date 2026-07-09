#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	dispn(in.encClamp(a2, a3, 0, 99));
}
