// 7:52
// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"

const int seg1[5][2] = {
	{ 0x21, 0x00 },
	{ 0x42, 0x00 },
	{ 0x00, 0x50 },
	{ 0x00, 0x0C },
	{ 0x00, 0x00 },
};

const int seg2[5][2] = {
	{ 0x30, 0x00 },
	{ 0x01, 0x01 },
	{ 0x00, 0x06 },
	{ 0x08, 0x08 },
	{ 0x00, 0x00 },
};

int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;

int idx = 4;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == LOW) {
		if (pretsw != tsw) {
			pretsw = tsw;
			idx = 4;
		}

		if (swps) {
			swps = false;
			if (++idx > 3) idx = 0;
		}

		if (phps) {
			phps = false;
			if (--idx < 0) idx = 3;
		}

		disp(seg1[idx][0], seg1[idx][1]);
	} else {
		if (pretsw != tsw) {
			pretsw = tsw;
			idx = 4;
		}

		if (swps) {
			swps = false;
			if (++idx > 3) idx = 0;
		}

		if (phps) {
			phps = false;
			if (--idx < 0) idx = 3;
		}

		disp(seg2[idx][0], seg2[idx][1]);
	}
}
