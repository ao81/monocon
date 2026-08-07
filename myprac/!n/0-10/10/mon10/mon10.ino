#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Enc enc(a1, a2);
Js js(a3, a4);
Seq q;

const char *seg4[4][3] = {
	{ ".", ".", "." },
	{ "", "", " .." },
	{ "   .", "   .", "   ." },
	{ "    ..", "", "" },
};

const char* seg8[8][3] = {
	{ ".", ".", "." },
	{ "", "", ".." },
	{ "", "", " .." },
	{ "", "", "  .." },
	{ "   .", "   .", "   ." },
	{ "   ..", "", "" },
	{ "    ..", "", "" },
	{ ".    .", "", "" },
};

int que[6] = {0};
int len = 2;
int cnt = 4;

void loop() {
	if (q) {
		if (q.in()) len = 2;
		len = enc.clampTo(len, 2, 6);
		dp.n(len);
		if (sw1.htol()) {
			cnt = ts ? 8 : 4;
			for (int i = 0; i < len; i++) que[i] = random(0, cnt);
			q.next();
		}
	}
	if (q) {
		static int idx = 0;
		static bool d = true;
		if (q.in()) idx = 0;
		if (d) {

		}
	}
}
