// 18:23

#define USE_TIMER3_ISR
#include "mono_con.h"

const int segpara1[3] = { 0x00, 0x77, 0x5b };
const int segpara2[4][2] = {
	{ 0x58, 0x61 },
	{ 0x54, 0x23 },
	{ 0x23, 0x54 },
	{ 0x43, 0x4c },
};
int idx = 4, dir = 1;
bool working = false;

int seg[2] = { 0x00, 0x00 };

int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;

word in, tc;

void onedisp(int n[2]) {
	static int pren[2] = { 0x00, 0x00 };
	if (memcmp(n, pren, sizeof(pren)) != 0) {
		memcpy(pren, n, sizeof(pren));
		disp(n[0], n[1]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (tsw == LOW) {
			if (sw == LOW && presw == HIGH) swps = true;
			if (ph == HIGH && preph == LOW) phps = true;
		}

		presw = sw;
		preph = ph;
	}

	if (tsw == LOW) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) pretsw = tsw;
		if (sw == LOW && ph == HIGH) {
			seg[0] = segpara1[1];
			seg[1] = segpara1[2];
		} else if (sw == LOW) {
			seg[0] = segpara1[1];
			seg[1] = segpara1[0];
		} else if (ph == HIGH) {
			seg[0] = segpara1[0];
			seg[1] = segpara1[2];
		} else {
			seg[0] = seg[1] = segpara1[0];
		}
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			idx = 4;
			working = false;
		}

		if (!working) {
			if (swps) {
				swps = false;
				idx = 0;
				tc = 500;
				dir = 1;
				working = true;
			}
		} else {
			if (phps) {
				phps = false;
				working = false;
				seg[0] = seg[1] = 0x00;
			} else {
				if (tc >= 500) {
					tc = 0;
					memcpy(seg, segpara2[idx], sizeof(seg));
					idx += dir;
					if (idx == 0 || idx == 3) dir = -dir;
				}
			}
		}
	}

	swps = phps = false;
	onedisp(seg);
}