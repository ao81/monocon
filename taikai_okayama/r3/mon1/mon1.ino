#define USE_TIMER3_ISR
#include "mono_con.h"

const int color[2] = { B100, B001 };

int tsw, sw1, sw2;
int pretsw, presw1, presw2;
bool sw1ps = false, sw2ps = false;

int cycle = 0;
int coloridx = 0;
int dir = 10;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1ps = true;
		if (sw2 == LOW && presw2 == HIGH) sw2ps = true;

		presw1 = sw1;
		presw2 = sw2;
	}

	in++;

	tc++;
}

void setup() {
	config_init();
	serial_init();

	disp(num[0], num[0]);

	tsw = digitalRead(_USER_CON_4PIN);
	sw1 = digitalRead(_USER_CON_5PIN);
	sw2 = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		coloridx = 0;
		dir = 10;
	} else {
		coloridx = 1;
		dir = -10;
	}

	if (sw1ps || sw2ps) {
		cycle += sw1ps ? dir : -dir;
		cycle = constrain(cycle, 0, 990);
		sw1ps = sw2ps = false;
		tc = 0;
		disp(num[cycle / 100], num[cycle / 10 % 10]);
	}

	if (cycle > 0) {
		int phase = tc % cycle;
		if (phase < cycle / 2) {
			lm.color.GBR = color[coloridx];
		} else {
			lm.color.GBR = B000;
		}
		led_stepmotor(lm.b8);
	} else {
		lm.color.GBR = B000;
		led_stepmotor(lm.b8);
	}
}