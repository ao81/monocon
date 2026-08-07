#include "monocon_chuugoku.h"

Enc enc(a3, a4);
Di sw1(d3), sw2(d2);
Seq q;
Iv v;

int spd = 0, ti = 1, nd = 0;
bool g = false;
int i = 0;
int data[3] = { 0, 1, 0 };
int tmp[3] = { 0, 1, 0 };

const char* mode[3] = {
	"spd",
	"ti",
	"nd",
};

void loop() {
	if (q) {
		if (q.in()) i = 0;
		i = enc.loopTo(i, 0, 2);
		if (sw1.htol()) q.next();
		if (sw2.held(2000, L)) { spd = nd = 0; ti = 1; }
		if (q.after(100)) g = false;
		if (g) led(G);
		else led(B);
	}
	if (q) {
		if (q.in()) {
			memcpy(tmp, data, sizeof(data));
		}
		if (i != 2) tmp[i] = enc.clampTo(tmp[i], (i == 0 ? 0 : 1), (i == 0 ? 255 : 30));
		else tmp[i] = enc.loopTo(tmp[i], 0, 3);
		if (sw1.htol()) { data[i] = tmp[i]; q.toa(0); g = true; }
		if (sw2.htol()) q.toa(0);
	}
	static Tog t;
	static Iv v;

	auto& si = sig(i);
	auto& sd = sig(data[i]);
	auto& st = sig(tmp[i]);

	if (si.change() || sd.change() || st.change()) {
		t.reset();
		v.reset(false);

		if (q.is(0)) dp.s(mode[i]);
		else dp.n(tmp[i]);
		t.flip();
	}

	if (v(700)) {
		if (t()) {
			dp.s(mode[i]);
		} else if (q.is(0)) {
			dp.n(data[i]);
		} else {
			dp.n(tmp[i]);
		}
	}
}
