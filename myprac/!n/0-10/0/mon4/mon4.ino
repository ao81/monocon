#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

void loop() {
	int a = ar(a1);
	if (a < 500) {
		if (a < 10) disp(seg[a], 0x00, 0x00);
		else if (a >= 100) disp(seg[a / 100], seg[(a / 10) % 10], seg[a % 10]);
		else disp(seg[a / 10], seg[a % 10], 0x00);
	} else dispOff();
}
