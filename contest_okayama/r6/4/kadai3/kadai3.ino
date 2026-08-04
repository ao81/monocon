#define USE_TIMER3_ISR
#include "mono_con.h"

const int seg1[4][2] = {
	{ 0x77, 0x5b },
	{ 0x77, 0x00 },
	{ 0x00, 0x5b },
	{ 0x00, 0x00 },
};

const int seg2[4][2] = {
	{ 0x58, 0x61 },
	{ 0x54, 0x23 },
	{ 0x23, 0x54 },
	{ 0x43, 0x4c },
};

const int ini[2] = { 0x00, 0x00 };

const int* s;
int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;
int idx = 0, dir = 1;

bool moving = false;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_3PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_5PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_3PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (tsw == HIGH) {
		moving = false;

		if (sw == LOW && ph == HIGH) s = seg1[0];
		else if (sw == LOW) s = seg1[1];
		else if (ph == HIGH) s = seg1[2];
		else s = seg1[3];
	} else {
		if (swps) {
			swps = false;
			moving = true;
		}

		if (phps) {
			phps = false;
			moving = false;
		}

		if (moving) {
			if (tc >= 500) {
				tc = 0;
				s = seg2[idx];
				idx += dir;
				if (idx == 0 || idx == 3) dir = -dir;
			}
		} else {
			idx = 0, dir = 1, tc = 500;
			s = ini;
		}
	}
	disp(s[0], s[1]);
}
