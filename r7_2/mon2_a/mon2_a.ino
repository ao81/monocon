#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledParam[3] = { B001, B100, B010 };
int idx = 0;

int tsw = HIGH;

word in, cnt;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		tsw = digitalRead(19);
	}

	in++;

	cnt++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (cnt > 1000) {
		cnt = 0;
		lm.color.GBR = ledParam[idx];
		if (++idx > 2) idx = 0;
	}

	led_stepmotor(lm.b8);
}