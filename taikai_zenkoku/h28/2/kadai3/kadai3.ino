#include "monocon_chuugoku.h"

void setup() {
	begin();
}

const int si[3] = { 10, 16, 2 };
int i = 0;

void loop() {
	static int cnt = 0;
	if (in.d(d2).htol) cnt++;
	de ts = in.d(d3);
	if (ts.ltoh && ++i > 2) i = 0;
	if (ts.htol) bz(1000, 1000);
	disp(seg[(cnt / (si[i] * si[i])) % si[i]], seg[(cnt / si[i]) % si[i]], seg[cnt % si[i]]);
}
