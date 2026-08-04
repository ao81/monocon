#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn[5][2] = {{ 0x30, 0x00 }, { 0x06, 0x00 }, { 0x00, 0x30 }, { 0x00, 0x06 }, { 0x00, 0x00 }};

int sw1, sw2;
int presw1, presw2;

word in;

ISR(TIMER3_COMPA_vect) {
	if (++in > 5) {
		in = 0;

		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	disp(num[10], num[10]);

	sw1 = presw1 = digitalRead(_USER_CON_5PIN);
	sw2 = presw2 = digitalRead(_USER_CON_3PIN);
}

void loop() {
	static int nowptn = 4;

	if (sw1 == LOW && presw1 == HIGH) {
		if (--nowptn < 0) nowptn = 3;
		disp(ptn[nowptn][0], ptn[nowptn][1]);
	} else if (sw2 == LOW && presw2 == HIGH) {
		if (++nowptn > 3) nowptn = 0;
		disp(ptn[nowptn][0], ptn[nowptn][1]);
	}

	presw1 = sw1;
	presw2 = sw2;
}
