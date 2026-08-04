// 12:45
// sw2無し

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int presw, preph;
int swps = false, phps = false;
bool cd = false;

int cnt = 0;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == HIGH && presw == LOW) swps = true;
		if (!cd && tsw == HIGH && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (!cd) {
			if (swps) {
				swps = false;
				if (cnt < 20) cnt++;
			}

			if (phps) {
				phps = false;
				cd = true;
				tc = 1000;
			}

		} else {
			lm.color.GBR = (cnt > 0) ? B001 : B000;

			if (tc >= 1000) {
				tc = 0;
				if (cnt > 0) {
					cnt--;
				}
			}
		}

		disp(num[cnt / 10], num[cnt % 10]);
	} else {
		cnt = 0;
		disp(0x00, 0x00);
		lm.color.GBR = B000;
		cd = false;
	}

	led_stepmotor(lm.b8);
}