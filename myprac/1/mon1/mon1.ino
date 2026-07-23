#include "monocon_chuugoku.h"
#define sc static

void setup() { begin(); }

void loop() {
	sc Seq q;
	sc long sum = 0;
	sc int angle = 0;

	de ts = in.d(d3);
	de sw = in.d(d2);
	de ph = in.d(d1, 50);

	if (q.now() != 5) sm.step(2);

	q.top();
	if (!ts && !q.is(0)) q.to(0);
	if (q()) {//0
		led(0);
		dispOff();
		if (ts) q.next();
		sum = 0;
		sm.to(0);
		dm.fr();
	}
	static int round = 3;
	static int r = 3;
	if (q()) {//1
		if (q.in()) {
			round = 3;
			in.c = 1;
		}
		led(0b010);
		int rr = in.encClamp(a1, a2, 1, 3);
		round = rr * 2 + 1;
		dispNum(round);
		if (sw.htol) {
			q.next();
			r = round;
		}
	}
	static int score;
	if (q()) {//2
		static int ra;
		static ti t;
		if (q.in()) {
			ra = random(1000, 3001);
			t.start(ra);
		}
		led(0);
		dm.drive(70);

		if (ph.ltoh && t.active()) {
			bz(200, 200);
			led(0b001);
			ra = random(1000, 3001);
			t.start(ra);
		}

		if (t.done()) {
			q.next();
		}
	}
	if (q()) {//3
		sc unsigned long t0;
		if (q.in()) {
			dm.fr();
			bz(2000, 50);
			t0 = millis();
		}
		led(0b100);
		score = millis() - t0;
		dispNum(score);
		if (ph.ltoh) {
			q.next();
			sum += score;
		}
	}
	if (q()) {//4
		if (q.in()) {
			if (--r > 0) q.to(2);
			else q.to(5);
		}
	}
	if (q()) {//5
		sc int avg = 0;
		if (q.in()) {
			dm.br();
			avg = sum / round;
			bz(2000, 100);
			sm.to(avg * 5);
		}
		if (avg < 250) {
			led(0b100);
		} else if (avg <= 400) {
			led(0b010);
		} else {
			led(0b001);
		}

		sm.step(2);

		dispNum(avg);
	}
}
