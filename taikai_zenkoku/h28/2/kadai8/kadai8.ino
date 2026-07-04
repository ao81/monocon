#include "monocon_chuugoku.h"

void setup() {
	begin();
}

const int sg[8][2] = {
	{ 0x30, 0x00 },
	{ 0x21, 0x00 },
	{ 0x01, 0x01 },
	{ 0x00, 0x03 },
	{ 0x00, 0x06 },
	{ 0x00, 0x0c },
	{ 0x08, 0x08 },
	{ 0x18, 0x00 },
};
int i = 0;

void loop() {
	static iv t;
	if (!in.d(d2)) {
		if (t(1000)) {
			if (!in.d(d3)) {
				if (++i > 7) i = 0;
			} else {
				if (--i < 0) i = 7;
			}
		}
	}
	disp(0x00, sg[i][0], sg[i][1]);

	if (in.d(d1).ltoh) {
		if (i < 4) sm.mv(SPR);
		else sm.mv(-SPR);
	}

	sm.step(2);
}
