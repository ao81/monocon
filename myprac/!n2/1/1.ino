#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2), ph(d4);

void loop() {
	static Seq q;
	if (q) {
		dp.off();
		if (ph.ltoh()) {
			q.next();
		}
	}
	if (q) {
		static int wait = 3000;
		static Sw s;
		if (q.in()) {
			wait = random(3000, 8000);
			s.start();
		}
		if (s >= wait) q.next();
		static Iv v;
		static const char ptn[10][3][9] = {
			{ ".", "", "" },
			{ "", ".", "" },
			{ "", "", "." },
			{ "", "", " ." },
			{ "", "", "  ." },
			{ "", "", "   ." },
			{ "", "   .", "" },
			{ "   .", "", "" },
			{ "    .", "", "" },
			{ "     .", "", "" }
		};
		static int i = 0;
		dp.art(ptn[i][0], ptn[i][1], ptn[i][2]);
		if (v(100)) i = (i + 1) % 10;
	}
	static uint8_t winner = 0;
	if (q) {
		if (sw1.htol()) { winner = 1; q.next(); }
		else if (sw2.htol()) { winner = 2; q.next(); }
	}
	if (q) {
		if (winner == 1) dp.s("_1_");
		else dp.s("_2_").art(".  .  .", "", ".  .  .");
		if (ph.ltoh()) q.to(0);
	}
}
