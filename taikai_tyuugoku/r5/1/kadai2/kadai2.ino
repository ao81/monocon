// 19:20
// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int phase = 0;
int cnt = 0;

bool moving = false;
int angle = 0;

word in, tc;

void cw() {
	if (--phase < 0) phase = 7;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 7) phase = 0;
	lm.color.SM = stepm_init(phase);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (!moving && sw == LOW && presw == HIGH) swps = true;
		if (!moving && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (tsw == LOW) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			cw();
			cnt++;
		}
		if (phps) {
			phps = false;
			ccw();
			cnt--;
		}

		moving = false;
		angle = 0;
	} else {
		if (!moving) {
			if (swps) {
				swps = false;
				moving = true;
				angle = 60;
			}

			if (phps) {
				phps = false;
				moving = true;
				angle = -60;
			}
		} else {
			if (tc > 10) {
				tc = 0;
				if (angle > 0) {
					cw();
					cnt++;
					angle--;
				} else if (angle < 0) {
					ccw();
					cnt--;
					angle++;
				} else {
					moving = false;
				}
			}
		}
	}

	if (cnt >= 0) {
		int c = cnt % 100;
		disp(num[c / 10], num[c % 10]);
		lm.color.GBR = B000;
	} else {
		int c = abs(cnt) % 100;
		disp(num[c / 10], num[c % 10]);
		lm.color.GBR = B001;
	}

	led_stepmotor(lm.b8);
}
