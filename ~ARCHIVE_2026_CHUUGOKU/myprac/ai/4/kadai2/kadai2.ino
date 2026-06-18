// 3:15

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

const int led[3] = { B001, B100, B010 };
int idx = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			if (++idx > 2) idx = 0;
		}

		if (phps) {
			phps = false;
			if (--idx < 0) idx = 2;
		}

		lm.color.GBR = led[idx];
	} else {
		if (swps || phps) swps = phps = false;

		lm.color.GBR = B000;
		idx = 0;
	}

	led_stepmotor(lm.b8);
}
