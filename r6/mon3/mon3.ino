#define USE_TIMER3_ISR
#include "mono_con.h"

const int a2[2] = { 0x77, 0x5B };
const int BLANK = 0x00;
const int ptn[4][2] = {
	{ 0x58, 0x61 },
	{ 0x54, 0x23 },
	{ 0x23, 0x54 },
	{ 0x43, 0x4C },
};

int idx = 0, dir = 1;
bool status = false;
bool tick = false;
bool idxChanged = false;

int TSW = HIGH, SW1 = HIGH, SW2 = HIGH;
int preTSW = HIGH, preSW1 = HIGH, preSW2 = HIGH;

word in = 0, wait = 0;

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;
		TSW = digitalRead(18);
		SW1 = digitalRead(19);
		SW2 = digitalRead(17);
	}
	in++;

	if (status) {
		if (++wait > 500) {
			wait = 0;
			tick = true;

			idx += dir;
			if (idx <= 0 || idx >= 3) dir = -dir;
			idx = (idx < 0) ? 0 : (idx > 3) ? 3 : idx;
			idxChanged = true;
		}
	} else {
		wait = 0;
		tick = false;
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	int tsw = TSW;
	int sw1 = SW1;
	int sw2 = SW2;
	bool sts = status;
	bool tck = tick;
	bool idxC = idxChanged;
	int  cidx = idx;
	tick = false;
	idxChanged = false;

	if (tsw == HIGH) {
		if (tsw != preTSW) disp(BLANK, BLANK);
		if (sw1 != preSW1 || sw2 != preSW2) {
			if (sw1 == LOW && sw2 == LOW)  disp(a2[0], a2[1]);
			else if (sw1 == LOW && sw2 == HIGH) disp(a2[0], BLANK);
			else if (sw1 == HIGH && sw2 == LOW)  disp(BLANK, a2[1]);
			else                                 disp(BLANK, BLANK);
		}

	} else {
		if (tsw != preTSW) {
			status = false;
			idx = 0;
			dir = 1;
			wait = 0;
			tick = false;
			sts = false;
			cidx = 0;
			disp(BLANK, BLANK);

		} else {
			if (!sts && sw1 == LOW && preSW1 == HIGH) {
				status = true;
				idx = 0;
				dir = 1;
				wait = 0;
				sts = true;
				cidx = 0;
				disp(ptn[0][0], ptn[0][1]);
			}

			if (sts && sw2 == LOW && preSW2 == HIGH) {
				status = false;
				idx = 0;
				dir = 1;
				wait = 0;
				sts = false;
				disp(BLANK, BLANK);
			}

			if (sts && idxC) {
				disp(ptn[cidx][0], ptn[cidx][1]);
			}

			if (!sts) {
				disp(BLANK, BLANK);
			}
		}
	}

	preTSW = tsw;
	preSW1 = sw1;
	preSW2 = sw2;
}