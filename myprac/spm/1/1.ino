#include "monocon_chuugoku.h"

void setup() {
	begin();
}

iv v;
bool move = true;

void loop() {
	if (move && v(10)) {
		sm.cw();
	}
	if (in.d(d2).htol) move = false;
}
