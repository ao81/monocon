#include "monocon_chuugoku.h"

void setup() {
	begin();
}

static const int per[5] = { 0, 25, 50, 75, 100 };
int i = 0;

void loop() {
	de sw = in.d(d2);
	if (sw.ltoh) {
		if (++i > 4) i = 0;
	}
	if (in.d(d3).ltoh) i = 0;
	led(0b001, per[i]);
	switch (per[i]) {
	case 0: dispOff(); break;
	case 100: disp(0x00, 0x5c, 0x54); break;
	default: dispn(per[i]);
	}
}
