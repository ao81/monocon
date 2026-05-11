#define USE_TIMER3_ISR
#include "mono_con.h"

const int led[3] = { B001, B100, B010 };
int idx = 0;
int cnt = 0;

int tsw;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tc >= 100) {
			tc = 0;
			if (++cnt > 99) cnt = 0;
			if (cnt % 10 == 0) {
				if (++idx > 2) idx = 0;
			}
		}
	}

	lm.color.GBR = led[idx];
	led_stepmotor(lm.b8);
	disp(num[cnt / 10], num[cnt % 10]);
}
