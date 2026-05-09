#define USE_TIMER3_ISR
#include "mono_con.h"

const int bz[8] = { 262, 294, 330, 349, 392, 440, 494, 523 };
int idx = 0, dir = 1;
int tsw, sw, ph;
int speed = 400;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);

	tc = speed;
}

void loop() {
	if (ph == HIGH) {

	}

	if (tsw == HIGH) {
		if (tc >= speed) {
			tc = 0;
			tone(BZ_PIN, bz[idx]);
			idx += dir;
			if (idx <= 0 || idx >= 7) dir = -dir;
		}
	} else {
		tc = speed;
		idx = 0;
		dir = 1;
		noTone(BZ_PIN);
	}
}