#include "monocon_chuugoku.h"

Di sw(d3), ts(d1);
const int b[3] = { 10, 16, 3 };

void loop() {
	static int cnt = 0, i = 0;;
	if (sw.htol()) cnt++;
	if (ts.ltoh()) i = (i + 1) % 3;
	dp.base(cnt, b[i]);
}
