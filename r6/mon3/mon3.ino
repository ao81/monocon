#define USE_TIMER3_ISR
#include "mono_con.h"

const int D1 = 18;
const int D2 = 19;
const int D3 = 17;

int TSW = 0, SW1 = 0, SW2 = 0;
int pTSW = 0, pSW1 = 0, pSW2 = 0;

bool mode = true;

const int ptn[4][2] = {
	{ 0x58, 0x61 },
	{ 0x54, 0x23 },
	{ 0x23, 0x54 },
	{ 0x43, 0x4c },
};

int ptnidx = 0, dir = 1;

word in, tc;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		pTSW = TSW, pSW1 = SW1, pSW2 = SW2;

		TSW = digitalRead(D1);
		SW1 = digitalRead(D2);
		SW2 = digitalRead(D3);
	}
	
	if (tc > 500) {
		tc = 0;
		disp(ptn[ptnidx][0], ptn[ptnidx][1]);
		if (++ptnidx > 3) ptnidx = 0;
	}

	in++;
	
	if (!mode) tc = 500;
	else tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (TSW == HIGH) {
		if (SW1 == LOW) {
			if (SW2 == LOW) {
				disp(0x77, num[2]);
			} else {
				disp(0x77, num[10]);
			}

		} else {
			if (SW2 == LOW) {
				disp(num[10], num[2]);
			} else {
				disp(num[10], num[10]);
			}
		}

	} else {
		if (TSW != pTSW) mode = false;

		if (!mode && SW1 == LOW && pSW1 == HIGH) {
			ptnidx = 0;
			dir = 1;
			mode = true;
		}

		if (mode && SW2 == LOW && pSW2 == HIGH) {
			mode = false;
		}

		if (mode) {


		} else {
			disp(0x00, 0x00);

		}
	}
}