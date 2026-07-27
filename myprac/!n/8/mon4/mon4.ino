#include "monocon_chuugoku.h"

const int note[3][2][8] = {
	{
		{ DO, RE, MI, FA, SO, FA, MI, RE },
		{ 500, 500, 500, 500, 500, 500, 500, 500 }
	},
	{
		{ DO, MI, SO, DO * 2, MI * 2, DO * 2, SO, MI },
		{ 500, 500, 500, 500, 500, 500, 500, 500 }
	},
	{
		{ DO, RE, MI, RE, MI, FA, MI, FA },
		{ 500, 500, 500, 500, 500, 500, 500, 500 }
	}
};

int m = 0;

Di sw1(d3), sw2(d2), ts(d1);

void loop() {
	static Seq q;
	if (q) {
		if (sw1.htol()) m = (m + 1) % 3;
		led();
		dp.n(m + 1);
		if (sw2.htol()) q.next();
	}
	if (q) {
		if (q.in()) bz.play(note[m][0], note[m][1], 8, ts);
		if (!bz.playing() || sw2.htol()) { bz.stop(); q.prev(); }
		led(B);
	}
}
