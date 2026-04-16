// 7:15

// ボリュームをjs左右で操作する

#define USE_TIMER3_ISR
#include "mono_con.h"

const int para1[6][2] = {{ 0x00, 0x00 }, { 0x08, 0x08 }, { 0x1C, 0x1C }, { 0x5C, 0x5C }, { 0x7E, 0x7E }, { 0x7F, 0x7F }};
const int para2[6][2] = {{ 0x00, 0x00 }, { 0x30, 0x00 }, { 0x79, 0x00 }, { 0x7F, 0x30 }, { 0x7F, 0x79 }, { 0x7F, 0x7F }};

const int lo = 0 + 200;
const int hi = 1023 - 200;

int xpos, prexpos, tsw;
int val = 0;

word in;

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
		xpos = analogRead(A1);
		tsw = digitalRead(_USER_CON_5PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	xpos = analogRead(A1);
	tsw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (xpos > hi && prexpos <= hi && val > 0) val--;
	if (xpos < lo && prexpos >= lo && val < 5) val++;

	if (tsw == HIGH) onedisp(para1[val]);
	else onedisp(para2[val]);

	prexpos = xpos;
}