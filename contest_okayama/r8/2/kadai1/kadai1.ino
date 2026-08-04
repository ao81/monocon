#define USE_TIMER3_ISR
#include "mono_con.h"

const int led[4] = { B000, B001, B010, B100 };
int idx = 0;

int tsw, sw;
int pretsw, presw;
bool swps = false;

bool move = false;

word tc;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	pretsw = !tsw;
	sw = presw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		idx = 0;
		move = false;
	}

	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			if (++idx > 3) idx = 0;
		}
	} else {
		if (swps) {
			swps = false;
			move = !move;
			idx = 0;
			tc = 500;
		}

		if (tc >= 500) {
			tc = 0;
			if (move && ++idx > 3) idx = 0;
		}
	}

	lm.color.GBR = led[idx];
	led_stepmotor(lm.b8);
}
