// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"
#define l lm.color.GBR

const int led[3] = { B001, B100, B010 };
int idx = 0;

int tsw, sw, ph;
int pretsw, presw;
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
	if (tsw == HIGH) {
		if (tsw != pretsw) pretsw = tsw;
		if (sw == LOW && ph == HIGH) l = B010;
		else if (sw == LOW) l = B001;
		else if (ph == HIGH) l = B100;
		else l = B000;
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			idx = 0;
		}

		if (swps) {
			swps = false;
			if (++idx > 2) idx = 0;
		}

		l = led[idx];
	}

	led_stepmotor(lm.b8);
}
