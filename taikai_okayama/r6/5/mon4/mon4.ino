#include "monocon_chuugoku.h"

Di ts(d1), sw1(d3), sw2(d2);

void loop() {
	static int ondo = 20;
	if (sw1.htol() && ondo > 16) ondo--;
	if (sw2.htol() && ondo < 40) ondo++;
	static bool tgl = true;
	static Iv t;
	if (!ts) {
		dp.n(ondo);
		tgl = true;
		t.reset();
	} else {
		if (tgl) {
			dp.s("__F");
		} else {
			int n = ondo * 9 / 5 + 32;
			if (n < 100) dp.n(n);
			else dp(0x00, seg[0] | 0x80, 0x38 | 0x80);
		}
		if (t(500)) tgl = !tgl;
	}
}
