#include "monocon_chuugoku.h"

void setup() {
	begin();
}

Di ts(d1), sw(d3), ph(d4);

void loop() {
	static int n = 0;
	if (sw.htol()) {
		if (ts) n++;
		else n--;
		if (n < 0) n = 20;
		if (n > 20) n = 0;
	}
	if (n == 0) led(0);
	else if (n <= 10) {
		static Iv t;
		static bool tgl = true;
		if (t(500)) {
			if (tgl) led(G);
			else led(0);
			tgl = !tgl;
		}
	} else if (n <= 19) {
		static Iv t;
		static bool tgl = true;
		if (t(100)) {
			if (tgl) led(R);
			else led(0);
			tgl = !tgl;
		}
	} else led(R);
	if (ph.held(2000, H)) n = 0;
	dp.n(n);
}
