#include "monocon_chuugoku.h"

Di sw(d3);
Sok sok(a1, 100);
int cm = sok.cm();

void loop() {
	cm = sok.cm();
	dp.n(cm);
	static Seq q;
	if (q) {
		led(R);
		if (cm > 45) q.to(2);
		else if (cm > 30) q.to(1);
	}
	if (q) {
		led(B);
		if (cm > 45) q.to(2);
		else if (cm < 30) {
			q.to(0);
			if (sw) bz(1000, 100);
		}
	}
	if (q) {
		led(G);
		if (cm < 30) {
			q.to(0);
			if (sw) bz(1000, 100);
		}
		else if (cm < 45) q.to(1);
	}
}
