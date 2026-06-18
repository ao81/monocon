#define USE_TIMER3_ISR
#include "mono_con.h"

const int pitch[8] = { 261, 293, 329, 349, 391, 440, 493, 523 };
int idx = 0;

int tsw, sw, ph;
int presw;
bool swps = false;

int phase = 0;
int angle = 0;

word in, tc, bz;

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (tsw == HIGH && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;

	if (bz > 0) {
		bz--;
		if (bz == 0) noTone(BZ_PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);

	disp(0x00, 0x00);
}

void loop() {
	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			angle += 10;
			tone(BZ_PIN, pitch[idx]);
			bz = 300;
			disp(num[idx + 1], 0x00);
			if (idx <= 2) lm.color.GBR = B001;
			else if (idx <= 4) lm.color.GBR = B100;
			else lm.color.GBR = B010;
			if (++idx > 7) idx = 0;
		}

		if (tc > 20) {
			tc = 0;

			if (angle > 0) {
				cw();
				angle--;
			} else if (angle < 0) {
				ccw();
				angle++;
			}
		}
	} else {
		if (ph == HIGH) {
			if (tc > 60) {
				tc = 0;
				ccw();
			}
		}
		lm.color.GBR = B000;
	}

	led_stepmotor(lm.b8);
}
