#define useir
#include "monocon_chuugoku.h"

void setup() {
	begin();
}

int dir = 1;
int b = 0;
int aa = 0;

void ir() {
	aa++;
}

void loop() {
	{
		static int n = 0;

		in sw1 = di(d3);
		in sw2 = di(d2);
		if (sw1.htol) n--;
		if (sw2.htol) n++;

		in enc = an.enc(a3, a4);
		n += enc.delta();

		in pr = an.pr(a2);
		if (pr) led(G);
		else led(R);

		// dp.n(n);
	}

	{
		if (aa >= 1000) {
			aa = 0;
			in sk = an.sok(a1);
			dp.n(sk);
		}
	}

	if (0) {
		in jo = an.joy(a3, a4);
		dp.n(jo.dir(8, 2));
	}

	if (0) {
		if (aa >= 100) {
			aa = 0;
			b += dir;
			if (b == 100) dir = -1;
			if (b == 0) dir = 1;
		}
		led(R, b);
		dp.s("end");
	}
}
