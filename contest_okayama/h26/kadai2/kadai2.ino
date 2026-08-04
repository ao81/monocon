// 16:17

// sw1,2をjs←,→とする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int segPara[5] = { 0x30, 0x79, 0x7F, 0x4F, 0x06 };
int index = 0;
bool first = true;

const int lo = 0 + 200;
const int hi = 1023 - 200;

int xpos, prexpos, jsps = 0;

word in, tc;

typedef enum {
	WAIT,
	SHIFT_R,
	SHIFT_L,
} Status;
Status st = WAIT;

void onedisp(int n, int m) {
	static int pren = 0x00, prem = 0x00;
	if (pren != n || prem != m) {
		pren = n, prem = m;
		disp(n, m);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		xpos = analogRead(A1);

		if (st == WAIT) {
			if (xpos > hi && prexpos <= hi) jsps = -1;
			if (xpos < lo && prexpos >= lo) jsps = 1;
		}
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	xpos = prexpos = analogRead(A1);
}

void loop() {
	switch (st) {
	case WAIT:
		if (first) {
			first = false;

		}

		if (jsps != 0) {
			if (jsps == 1) {
				st = SHIFT_R;
			} else if (jsps == -1) {
				st = SHIFT_L;
			}

			first = true;
			jsps = 0;
		}

		break;
	case SHIFT_R:
		if (first) {
			first = false;
			index = 0;
			tc = 300;
		}

		if (tc > 300) {
			tc = 0;
			if (index <= 4) {
				disp(segPara[index], 0x00);
			} else if (index <= 9) {
				disp(0x00, segPara[index - 5]);
			} else {
				disp(0x00, 0x00);
				st = WAIT;
			}

			index++;
		}

		break;
	case SHIFT_L:
		if (first) {
			first = false;
			index = 9;
			tc = 0;
		}

		if (tc > 300) {
			tc = 0;
			if (index >= 5) {
				disp(0x00, segPara[index - 5]);
			} else if (index >= 0) {
				disp(segPara[index], 0x00);
			} else {
				disp(0x00, 0x00);
				st = WAIT;
			}

			index--;
		}

		break;
	}
}