#include "monocon_chuugoku.h"

void setup() {
	begin();
}

const int lh[2] = { 0x38, 0x76 };

void loop() {
	de ts = in.d(d3);
	de sw = in.d(d2);
	de ph = in.d(d1);

	if (ph.ltoh) {
		if (ts) bz(2000, 2000);
		else bz(1000, 2000);
	}

	static bool blink = false;
	static bool tgl = true;
	static iv t;

	if (!sw) {
		blink = true;
	} else {
		blink = false;
		tgl = true;
		t.reset();
	}

	if (blink) {
		if (t(500)) tgl = !tgl;
	}

	if (tgl) {
		if (ph) disp(0x00, lh[ts], lh[sw], 255, 30);
		else disp(0x00, lh[ts], lh[sw]);
	} else {
		dispOff();
	}
}
