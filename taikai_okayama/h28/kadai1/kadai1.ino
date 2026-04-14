// 8:46

// sw2をphとする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn[2] = { 0x5C, 0x63 }; //  off, on
int dispptn[2] = { 0x00, 0x00 };

int sw, ph, tsw;

word in;

void onedisp(int n[2]) {
	static int pren[2] = { 0x00, 0x00 };
	if (n != pren) {
		memcpy(pren, n, sizeof(pren));
		disp(n[0], n[1]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
		tsw = digitalRead(_USER_CON_5PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
	tsw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	dispptn[0] = dispptn[1] = 0x00;
	if (sw == LOW) dispptn[0] |= ptn[(int)tsw];
	if (ph == HIGH) dispptn[1] |= ptn[(int)tsw];
	onedisp(dispptn);
}