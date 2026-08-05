#include "monocon_chuugoku.h"

Di sw1(d1), sw2(d2), sw3(d3), ts(d4);
Js js(a1, a2);
Enc enc(a3, a4);
Iv v;
Ti t, t2;
int dir = -1;
int bpm = 120;
int beat = 4;
int now = 0;
bool on = false;

void loop() {
	if (sw1.htol()) on = !on;
	if (sw2.htol()) beat = wrap(beat + 1, 2, 4);
	if (sw3.htol()) { bpm = 120; beat = 4; on = false; }

	dir = js.dir(8, 2);
	static const int pitch[8] = { DO, RE, MI, FA, SO, RA, SI, DO * 2 };
	if (dir != -1) bz(pitch[dir]);
	else bz.off();

	bpm = enc.clampTo(bpm, 40, 240);

	if (sig(beat).change()) t2.start(1000);
	if (t2.done()) t2.stop();
	if (t2.active()) dp.n(beat);
	else dp.n(bpm);

	if (on && v(60000 / bpm)) {
		t.start(80);
		if (now == 0) led(R);
		else led(G);
	}
	if (t.done()) led();
}
