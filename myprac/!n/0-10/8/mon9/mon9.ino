#define dbg
#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);
Seq q;
int phase = 0;
int ful = 0;
int score[3] = { 0 };

void loop() {
	if (q) {
		if (q.in()) { phase = 0; ful = 0; }
		led();
		dp.off();
		if (sw1.htol()) q.next();
	}
	if (q) {
		static Ti t;
		if (q.in()) { t.start(random(1000, 3000 + 1)); led(0); dp.off(); }
		if (sw1.htol()) q.to(4);
		if (t.done()) q.next();
	}
	static Sw s;
	if (q) {
		led(G);
		if (q.in()) { s.reset(); s.start(); }
		if (sw1.htol()) { s.stop(); q.next(); }
	}
	if (q) {
		dp.n(s.ms());
		static Ti t;
		if (q.in()) t.start(2000);
		if (t.done()) q.to(5);
	}
	if (q) {
		static Ti t;
		if (q.in()) { bz(800, 1000); t.start(2000); ful++; }
		dp.s("FUL");
		s.reset();
		if (t.done()) q.to(5);
	}
	if (q) { // 5
		score[phase] = s.ms();
		if (phase >= 2) q.next();
		else {
			phase++;
			q.to(1);
		}
	}
	if (q) {
		static int sum = 0, avg = 0;
		if (q.in()) {
			// DV(phase);
			sum = 0;
			for (int i = 0; i < 3; i++) sum += score[i];
			if (ful == 3) avg = 0;
			else avg = sum / (3 - ful);
		}
		static Iv v;
		static Tog t;
		if (v(500)) {
			if (t()) dp.n(avg);
			else dp.off();
		}
	}
	if (sw2.htol()) q.to(0);
}
