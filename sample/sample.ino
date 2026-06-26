#include "monocon_chuugoku.h"

static int color[3] = { B001, B010, B100 };
volatile word t = 10000, s = 0;
int c = 0;

void isr() {
	t++;
	s++;
	in.fen(a1, a2);
	if (cnt < 0) cnt = 999;
	if (cnt > 999) cnt = 0;
}

void setup() {
	begin();
}

void loop() {
	static int idx = 0;
	if (t >= 10000) {
		t = 0;
		led(color[idx]);
		if (++idx > 2) idx = 0;
		bz(2000, 50);
		if (++c > 999) c = 0;
	}

	if (c % 2 == 0) {
		dcm.cw(50);
	} else {
		dcm.br();
	}

	if (c % 2 == 0) {
		if (s >= 20) {
			s = 0;
			spm.cw();
		}
	}

	disp(seg[cnt / 100], seg[(cnt / 10) % 10], seg[cnt % 10]);
}
