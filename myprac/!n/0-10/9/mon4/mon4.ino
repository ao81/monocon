#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);
Js js(a3, a4);
Sig g(30);
Seq q;
Iv v;
Ti t;

const char* ptn[][3] = {
	{"", ".", ""}, // 上
	{"", "", ".."},
	{"", "", " .."}, // 右
	{"", "", "  .."},
	{"", "   .", ""}, // 下
	{"   ..", "", ""},
	{"    ..", "", ""}, // 左
	{".    .", "", ""},
};

const int oknote[] = { DO, MI, SO };
const int ngnote[] = { SO, MI, DO };
const int notems[] = { 90, 90, 160 };

int que[8];
int n = 2, i = 0, cleared = 0;
bool mirror = false, armed = false;

void startGame() {
	mirror = sw2;
	n = 2;
	cleared = 0;
	for (int& d : que) d = random(0, 8);
	led();
	q.toa(1);
}

void loop() {
	if (q) {
		if (q.in()) dp.s("go_");
		if (sw1.htol()) startGame();
	}
	if (q) {
		if (q.in()) { i = 0; v.reset(); }
		if (v(500)) {
			if (i < n) {
				const int d = mirror ? ((8 - que[i]) & 7) : que[i];
				dp.art(ptn[d]);
				i++;
			} else {
				q.next();
			}
		}
		if (q.out()) dp.off();
	}
	if (q) {
		if (q.in()) {
			i = 0;
			const int d = js.dir(8, 2, mirror);
			g.reset(d);
			armed = d == -1;
			t.start(3000);
		}
		const int d = g(js.dir(8, 2, mirror));
		if (d == -1) armed = true;
		dp.n((t.remain() + 999) / 1000);
		if (t.done()) {
			q.toa(4);
		} else if (armed && d != -1 && g.from(-1)) {
			armed = false;
			if (d != que[i]) {
				t.stop();
				q.toa(4);
			} else if (++i >= n) {
				t.stop();
				cleared++;
				if (n == 8) {
					q.next();
				} else {
					n++;
					q.prev();
				}
			} else {
				t.start(3000);
			}
		}
	}
	if (q) {
		if (q.in()) {
			dp.s("clr");
			led(G);
			bz.play(oknote, notems, 3);
		}
		if (!bz.playing() && sw1.htol()) startGame();
	}
	if (q) {
		if (q.in()) {
			t.stop();
			dp.n(cleared);
			led(R);
			bz.play(ngnote, notems, 3);
		}
		if (!bz.playing() && sw1.htol()) startGame();
	}
}
