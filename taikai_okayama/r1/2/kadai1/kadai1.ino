// 6:36

#define USE_TIMER3_ISR
#include "mono_con.h"

const int loEdge = 0 + 200;
const int hiEdge = 1023 - 200;
const int ptn[3] = { 0x08, 0x40, 0x01 };

int sw, ypos;

word in;

void onedisp(int n) {
	static int pren = 0x00;
	if (pren != n) {
		pren = n;
		disp(0x00, n);
	}
}

ISR (TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ypos = analogRead(A2);
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	int dispptn = 0x00;
	if (sw == LOW) dispptn |= ptn[0];
	if (ypos < loEdge) dispptn |= ptn[1];
	if (ypos > hiEdge) dispptn |= ptn[2];
	onedisp(dispptn);
}