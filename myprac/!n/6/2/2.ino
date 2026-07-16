#include "monocon_chuugoku.h"

void setup() {
	begin();
}

vr v(a3, 0, 500);
Iv t;

void loop() {
	if (t(50)) {
		static int l[3] = { R, B, G };
		int vto = v.to(0, 100);
		dp.n(vto);
		int i = clamp(map(vto, 0, 100, 0, 3), 0, 2);
		led(l[i]);
	}
}
