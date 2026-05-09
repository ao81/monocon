#define USE_TIMER3_ISR
#include "mono_con.h"

int sw;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		sw = digitalRead(_USER_CON_4PIN);
	}
}

void setup() {
	config_init();
	serial_init();
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (sw == LOW) lm.color.GBR = B011;
	else lm.color.GBR = B000;
	led_stepmotor(lm.b8);
}
