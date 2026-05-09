#define USE_TIMER3_ISR
#include "mono_con.h"

const int bz[8] = { 262, 294, 330, 349, 392, 440, 494, 523 };
int idx = 0;

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
	tc = 400;
}

void loop() {
	if (tsw == HIGH) {
		if (tc >= 400) {
			tc = 0;
			if (++idx > 7) idx = 0;
			tone(BZ_PIN, bz[idx]);
		}
	} else {
		noTone(BZ_PIN);
		idx = -1;
	}
	disp(num[(idx + 1) / 10], num[(idx + 1) % 10]);
}