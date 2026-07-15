#include "monocon_chuugoku.h"

void setup() {
	begin();
}

di sw1(d3), sw2(d2), ph(d4), ts(d1);
enc en(a3, a4);
pr p(a2);
sok so(a1);

void loop() {
#if 1
	if (ts == H) {
		uint8_t l = 0;
		if (sw1 == L) l |= B;
		if (sw2 == L) l |= R;
		if (ph == H) l |= G;
		led(l, 10);

		dp.off();

	} else {
		static const uint8_t color[3] = { G, B, R };
		static int idx = 0;
		idx = en.loopTo(0, 2);
		led(color[idx]);

		dp.n(so.cm);

		if (!p) bz(2000);
		else bz.off();
	}
#else
	if (sw1 == L) led(G);
	else led(0);
#endif
}
