#include "monocon_chuugoku.h"

Di ts(d1), sw1(d3), sw2(d2);

void loop() {
	static int i = 0;
	if (ts) {
		uint8_t l = 0;
		if (!sw1) l |= R;
		if (!sw2) l |= G;
		if (!sw1 && !sw2) l |= B;
		led(l);
		i = 0;
	} else {
		static const int l[3] = { R, G, B };
		if (sw1.htol()) i = (i + 1) % 3;
		led(l[i]);
	}
}
