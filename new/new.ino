#define useir
#include "monocon_chuugoku.h"

void setup() {
	begin();
}

int dir = 1;
int b = 0;
int n = 0;

void ir() {
	n++;
}

void loop() {
	static int n = 0;

	in sw = d(d3);
	if (sw.htol) n++;

	in enc = a.enc(a3, a4);
	n += enc.delta();

	in pr = a.pr(a2);
	if (pr) led(G);
	else led(R);

	in jo = a.joy(a3, a4);
	dp.n(jo.dir(8, 2));

	if (0) {
		if (n >= 100) {
			n = 0;
			b += dir;
			if (b == 100) dir = -1;
			if (b == 0) dir = 1;
		}
		led(R, b);
		dp.s("end");
	}
}
