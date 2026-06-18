// 10:38

#define USE_TIMER3_ISR
#include "mono_con.h"

const int pitch[8] = { 261, 293, 329, 349, 391, 440, 493, 523 };
int idx = 0;

int tsw, sw;
int presw;
bool swps = false;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (tsw == LOW && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (tsw == HIGH) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	noInterrupts();
	int t = tc;
	interrupts();

	if (tsw == HIGH) {
		if (t >= 400) {
			tc = 0;
			disp(num[idx + 1], 0x00); 
			tone(BZ_PIN, pitch[idx]);
			if (idx <= 2) lm.color.GBR = B001;
			else if (idx <= 4) lm.color.GBR = B100;
			else lm.color.GBR = B010;
			if (++idx > 7) idx = 0;
		}
	} else {
		if (swps) {
			swps = false;
			idx = 0;
			lm.color.GBR = B001;
			noTone(BZ_PIN);
		}
		tc = 400;
	}

	led_stepmotor(lm.b8);
}
