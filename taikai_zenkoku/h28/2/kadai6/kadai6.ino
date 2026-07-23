#include "monocon_chuugoku.h"

static int dspd[4] = { -1, 50, 100, 150 };
const int sspd[4] = { -1, 20, 10, 3 };
int di = 0;
int si = 0;

void setup() {
	begin();
}

void loop() {
	de ts = in.d(d3);
	de sw = in.d(d2);
	de ph = in.d(d1);

	if (sw.htol) {
		if (!ts) {
			if (++di > 3) di = 0;
		} else {
			if (++si > 3) si = 0;
		}
	}

	if (ts) di = 0;
	else si = 0;

	if (di > 0) dm.drive(ph ? -dspd[di] : dspd[di]);
	else dm.fr();
	if (si > 0) sm.drive(ph ? -sspd[si] : sspd[si]);
}
