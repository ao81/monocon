#include "monocon_chuugoku.h"

Di sw(d3);
Js js(a3, a4);
int n = 0, i = 0;

void loop() {
	n = js.dir(4, 2);
	auto& s = sig(n);
	static const int angles[4] = { 0, 90, 180, 270 };
	if (s.change() && s != -1) i = s;
	sm.abso(angles[i], SHORT, CW);
	sm.update(2);
	if (sw.htol()) { sm.zero(); i = 0; }
	if (!sm.busy()) {
		sm.fr();
		dp.n(angles[i]);
	} else {
		static const char anim[10][3][10] = {
			{".", "", ""},
			{"", ".", ""},
			{"", "", "."},
			{"", "", " ."},
			{"", "", "  ."},
			{"", "", "   ."},
			{"", "   .", ""},
			{"   .", "", ""},
			{"    .", "", ""},
			{"     .", "", ""},
		};
		static int a = 0;
		static Iv v;
		if (v(80)) {
			dp.art(anim[a][0], anim[a][1], anim[a][2]);
			a = wrap(a + sm.dir(), 0, 9);
		}
	}
}
