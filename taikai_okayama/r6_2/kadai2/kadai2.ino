// 7:07

// sw2 => ph

#define USE_TIMER3_ISR
#include "mono_con.h"

#define color lm.color.GBR
const int ledParam[3] = { B001, B101, B010 };
int index = 0;

int tsw, sw, ph, pretsw, presw;
bool swps = false;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (tsw == LOW && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	color = B000;
	if (tsw == HIGH) {
		if (pretsw != tsw) pretsw = tsw;
		if (sw == LOW && ph == HIGH) color |= B010;
		else {
			if (sw == LOW) color |= B001;
			if (ph == HIGH) color |= B100;
		}
	} else {
		if (pretsw != tsw) {
			pretsw = tsw;
			index = 0;
		}
		if (swps) {
			swps = false;
			if (++index > 2) index = 0;
		}
		color = ledParam[index];
	}
	led_stepmotor(lm.b8);
}