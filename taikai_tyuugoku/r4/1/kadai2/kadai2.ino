// 5:11
// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"

const int seg[9][2] = {
	{ 0x20, 0x00 },
	{ 0x02, 0x00 },
	{ 0x00, 0x20 },
	{ 0x00, 0x02 },
	{ 0x10, 0x00 },
	{ 0x04, 0x00 },
	{ 0x00, 0x10 },
	{ 0x00, 0x04 },
	{ 0x00, 0x00 },
};
int idx = 8;

int sw, ph;
int presw, preph;
bool swps = false, phps = false;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

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

	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (swps) {
		swps = false;
		if (--idx < 0) idx = 7;
	}

	if (phps) {
		phps = false;
		if (++idx > 7) idx = 0;
	}

	disp(seg[idx][0], seg[idx][1]);
}
