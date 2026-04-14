// 6:51

#define USE_TIMER3_ISR
#include "mono_con.h"

const int offptn[6] = { 0x00, 0x08, 0x1C, 0x5C, 0x7E, 0x7f };
const int loedge = 0 + 200;
const int hiedge = 1023 - 200;

int tsw, xpos, prexpos;
int xps = 0;
int value = 0;

int dispseg[2] = { 0x00, 0x00 };

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

		tsw = digitalRead(_USER_CON_5PIN);
		xpos = analogRead(A1);

		if (xpos > hiedge && prexpos <= hiedge) xps = -1;
		if (xpos < loedge && prexpos >= loedge) xps = 1;

		prexpos = xpos;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	xpos = analogRead(A1);
}

void loop() {
	dispseg[0] = dispseg[1] = 0x00;

	if (xps == -1) {
		xps = 0;
		if (value > 0) value--;
	}
	if (xps == 1) {
		xps = 0;
		if (value < 5) value++;
	}

	if (tsw == HIGH) {
		dispseg[0] = 0x00;
		dispseg[1] = num[value];
	} else {
		dispseg[0] = dispseg[1] = offptn[value];
	}

	onedisp(dispseg);
}