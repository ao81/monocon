#include "monocon_chuugoku.h"

typedef enum {
	menu = -1,
	m0,
	m1,
	m2,
} status;
status st = menu;

int tc = 0;
void isr() {
	if (st == m0) tc++;
}

void setup() {
	begin();
}

Timer t;

void loop() {
	de ts = in.d(d3);
	de sw = in.d(d2);
	de ph = in.d(d1);
	int re = in.enc(a1, a2);
	static int n = 0;

	if (ts == LOW) { // wait
		st = menu;
		in.encSet(0);
		n = 0;
		dispOff();
		led(0);

	} else { // move
		switch (st) {
		case menu:
			n = abs(re) % 3;
			disp(0x73, 0x00, seg[n]);
			led(0b111);
			if (sw.htol) {
				st = (status)n;
				sm.zero();
				in.encReset();
				if (st == m2) {
					t.start(random(2000) + 1000);
				}
			}

			break;
		case m0: {
			const int round = 128;
			const int one = 2048 / round;

			int idx = ((re % round) + round) % round;
			long tg = (long)idx * one;

			sm.seek(tg, 2048);
			sm.step(2);

			dispNum(idx);
			led(sm.moving() ? 0b010 : 0b100);

			break;
		}
		case m1:
			if (re > 20) {
				re = 20;
				in.encSet(20);
			}
			if (re < -20) {
				re = -20;
				in.encSet(-20);
			}

			dm.drive(deadband(map(re, -20, 20, -100, 100), 0, 25));

			dispNum(re);

			break;
		case m2: {
			static int m;
			if (t.active()) {
				led(0b010);
				if (sw.htol) {
					bz(800, 1000);
					st = menu;
				}
			}
			if (t.done()) {
				led(0b100);
				m = millis();
			}
			if (!t.active()) {
				if (sw.htol) {
					dispNum(millis() - m);
					bz(2000, 1000);
				}
			}

			break;
		}
		}
	}
}
