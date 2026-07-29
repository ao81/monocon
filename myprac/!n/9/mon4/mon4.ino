// 途中まで

#include "monocon_chuugoku.h"

Di sw1(d3), sw2(d2);
Js js(a3, a4);
Sig g;
Seq q;
Iv v;

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

int n = 2, i = 0;
int que[9] = {};

void loop() {
	if (q) { // 問題の初期化
		if (q.in()) {
			for (int i = 0; i < 9; i++) {
				que[i] = random(0, 8);
			}
			q.next();
		}
	}
	if (q) { // 出題
		if (q.in()) {
			v.reset();
			i = 0;
		}
		if (v(500)) {
			if (i < n) {
				dp.art(ptn[que[i]]);
				i++;
			} else {
				q.next();
			}
		}
		if (q.out()) i = 0;
	}
	if (q) { // 回答
		g(js.dir(8, 2));
		if (g.change() && g != -1) {
			if (g == que[i++]) {
				q.restart();
			} else {
				q.tor(2);
			}
		}
		if (q.after(3000 * n)) {

		}
		if (i >= n) {
			q.prev();
			if (++n > 8) q.next();
		}
	}
	if (q) { // 成功

	}
	if (q) { // 失敗

	}
}
