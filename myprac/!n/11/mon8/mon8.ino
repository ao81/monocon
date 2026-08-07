#include "monocon_chuugoku.h"

Js js(a3, a4);
int n;

void loop() {
	n = js.dir(4, 2);
	if (n != -1) dp(num[n], 0, num[n]);
	else dp.off();
}
