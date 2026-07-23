#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn[5][2] = {
	{ 0x30, 0x00 },
	{ 0x06, 0x00 },
	{ 0x00, 0x30 },
	{ 0x00, 0x06 },
	{ 0x00, 0x00 },
};

int tsw, sw1, sw2;
int pretsw, presw1, presw2;
bool sw1ps = false, sw2ps = false;

word in;

ISR (TIMER3_COMPA_vect) {
	if (++in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1ps = true;
		if (sw2 == LOW && presw2 == HIGH) sw2ps = true;

		presw1 = sw1;
		presw2 = sw2;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_4PIN);
	sw1 = digitalRead(_USER_CON_5PIN);
	sw2 = digitalRead(_USER_CON_3PIN);
}

void loop() {
	static int idx = 4;
	bool update = false;

	if (sw1ps) {
		sw1ps = false;
		if (--idx < 0) idx = 3;
		update = true;

	} else if (sw2ps) {
		sw2ps = false;
		if (++idx > 3) idx = 0;
		update = true;

	}

	if (update) {
		disp(ptn[idx][0], ptn[idx][1]);
	}
}