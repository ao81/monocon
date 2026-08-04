#define USE_TIMER3_ISR
#include "mono_con.h"

const int led1[4] = { B001, B010, B100, B000 };
const int led2[3] = { B001, B010, B100 };
int idx = 0;

int tsw, sw;
int pretsw, presw;
bool swps = false;

bool moving = false;

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
	sw = presw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		idx = 0;
	}

	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			if (++idx > 3) idx = 0;
		}
		lm.color.GBR = led1[idx];
		moving = false;
	} else {
		if (swps) {
			swps = false;
			moving = !moving;
			tc = 500;
		}

		if (moving) {
			if (tc >= 500) {
				tc = 0;
				lm.color.GBR = led1[idx];
				if (++idx > 3) idx = 0;
			}
		} else {
			lm.color.GBR = B000;
		}
	}

	led_stepmotor(lm.b8);
}
