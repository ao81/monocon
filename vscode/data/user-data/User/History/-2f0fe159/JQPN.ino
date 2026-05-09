#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledpara[3] = { B001, B101, B100 };
int idx = 0;

int sw, presw;
bool swps = false;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		sw = digitalRead(_USER_CON_4PIN);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}
}

void setup() {
	config_init();
	serial_init();
	sw = presw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (swps) {
		swps = false;
		if (++idx > 2) idx = 0;
	}
	lm.color.GBR = ledpara[idx];
	led_stepmotor(lm.b8);
}