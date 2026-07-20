#include "monocon_chuugoku.h"

Di ts(d1), sw1(d3), sw2(d2);
int i = 0;

void loop() {
	if (ts.change()) i = 0;
	if (!ts) {
		static const int p[5][2] = {
			{ 0x00, 0x00 },
			{ 0x21, 0x00 },
			{ 0x42, 0x00 },
			{ 0x00, 0x50 },
			{ 0x00, 0x0c },
		};
		if (sw1.htol()) {
			if (++i > 4) i = 1;
		}
		if (sw2.htol()) {
			if (--i < 1) i = 4;
		}
		dp(0x00, p[i][0], p[i][1]);
	} else {
		static const int p[5][2] = {
			{ 0x00, 0x00 },
			{ 0x30, 0x00 },
			{ 0x01, 0x01 },
			{ 0x00, 0x06 },
			{ 0x08, 0x08 },
		};
		if (sw1.htol()) {
			if (++i > 4) i = 1;
		}
		if (sw2.htol()) {
			if (--i < 1) i = 4;
		}
		dp(0x00, p[i][0], p[i][1]);
	}
}
