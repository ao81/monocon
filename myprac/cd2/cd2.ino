#define USE_TIMER3_ISR
#include "mono_con.h"

int sw, presw;
bool swps = false;
int cd = -1;
word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}

	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);

	disp(0x00, num[0]);
}

void loop() {
	if (swps) {
		swps = false;
		cd = 3000;
	}

	if (cd > 0) {
		int sec = (cd + 999) / 1000;
		disp(0x00, num[sec]);
	} else if (cd == 0) {
		disp(0x00, num[0]);
	}
}
