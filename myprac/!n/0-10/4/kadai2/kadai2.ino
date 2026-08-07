#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	static int cnt = 0;
	if (in.d(d2).htol) cnt = (cnt + 1) % 10;
	dispn(cnt);
}
