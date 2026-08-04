// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"

const int led1[2] = { 0x77, 0x5b };
const int led2[4][2] = {
	{ 0x58, 0x61 },
	{ 0x54, 0x23 },
	{ 0x23, 0x54 },
	{ 0x43, 0x4c },
};
int idx = 0, dir = 1;

bool mv = false;

int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (tsw == LOW && sw == LOW && presw == HIGH) swps = true;
		if (tsw == LOW && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}
	if (tsw == LOW) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) pretsw = tsw;
		int l[2] = { 0x00, 0x00 };
		if (sw == LOW) l[0] |= led1[0];
		if (ph == HIGH) l[1] |= led1[1];
		disp(l[0], l[1]);
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			mv = false;
		}
		if (swps) {
			swps = false;
			mv = true;
		}
		if (phps) {
			phps = false;
			mv = false;
		}
		if (mv) {
			if (tc >= 500) {
				tc = 0;
				disp(led2[idx][0], led2[idx][1]);
				idx += dir;
				if (idx == 0 || idx == 3) dir = -dir;
			}
		} else {
			idx = 0;
			dir = 1;
			tc = 500;
			disp(0x00, 0x00);
		}
	}
}
