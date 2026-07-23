#define USE_TIMER3_ISR
#include "mono_con.h"

const int led[7][2] = {
	{ B100, 5000 },
	{ B000, 500 },
	{ B100, 500 },
	{ B000, 500 },
	{ B100, 500 },
	{ B101, 2000 },
	{ B001, 5000 },
};
int idx = 0;

volatile int tsw, sw;
int pretsw, presw;
volatile bool swps = false;
volatile word in, tc, cd;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}

	if (tc > 0) tc--;
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	// Serial.begin(9600);
}

void loop() {
	if (tsw == HIGH) {
		if (pretsw != tsw) {
			pretsw = tsw;
			tc = led[idx][1];
			idx = 0;
			lm.color.GBR = led[idx][0];
			// Serial.println(lm.color.GBR);
		}

		if (swps) {
			swps = false;
			if (lm.color.GBR == B100) {
				idx = 5;
				tc = led[idx][1];
				lm.color.GBR = led[idx][0];
			}
		}

		if (tc == 0) {
			if (++idx > 6) idx = 0;
			tc = led[idx][1];
			lm.color.GBR = led[idx][0];
		}

		if (idx >= 1 && idx <= 4) {
			disp(num[0], num[0]);
		} else if (tc > 0) {
			int s = (tc + 999) / 1000;
			disp(num[s / 10], num[s % 10]);
		} else disp(num[0], num[0]);

	} else {
		if (pretsw != tsw) {
			pretsw = tsw;
			lm.color.GBR = B000;
			disp(num[0], num[0]);
		}

		if (swps) {
			swps = false;
		}
	}

	led_stepmotor(lm.b8);
}
