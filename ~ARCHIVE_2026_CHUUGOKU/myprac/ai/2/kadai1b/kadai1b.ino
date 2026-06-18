// 10:40

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int presw;
bool swps = false;

int angle = 0, phase = 0;
int total = 0;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (angle == 0 && tsw == HIGH && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (tsw == HIGH) tc++;
}

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == LOW) {
		lm.color.GBR = B100;
		angle = 0;
		tc = 30;
	} else {
		lm.color.GBR = B001;

		if (swps) {
			swps = false;
			if (ph == LOW) angle = 45;
			else angle = -45;
		}

		if (tc >= 30) {
			tc = 0;

			if (angle > 0) {
				angle--;
				if (total < 99) { total++; cw(); }
				else angle = 0;
			} else if (angle < 0) {
				angle++;
				if (total > -99) { total--; ccw(); }
				else angle = 0;
			}
		}
	}

	int t = abs(total);
	disp(num[t / 10], num[t % 10]);

	led_stepmotor(lm.b8);
}
