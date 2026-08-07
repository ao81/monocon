#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);
Sok sok(a1); Enc enc(a3, a4);

int goal = 20;

void loop() {
	static Seq q;
	if (q) {
		goal = enc.clampTo(goal, 8, 40);
		dp.n(goal * 10);
		if (sw1.htol()) { q.next(); }
		if (q.out()) { goal *= 10; }
	}
	if (q) {
		int mm = sok.mm;
		int diff = mm - goal;
		dp.n(diff);
		if (abs(diff) >= 50) led(R);
		else if (abs(diff) >= 21) led(B);
		else led(G);
	}
}
