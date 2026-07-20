#include "monocon_chuugoku.h"

Di ts(d1);
int cnt = 0, n = 0;
Iv v;
Tog t;

void loop() {
	if (ts.change()) { n = ++cnt; v.reset(true); }
	if (v(100)) if (t() && n > 0) { led(cnt % 2 ? G : R); n--; } else { led(0); }
}
