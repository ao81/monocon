#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
int swps = false, phps = false;
bool r = true;

int move = 0;
bool force = false;

int cnt = 0;
word tc = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	lm.led(B010);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw != presw) swps = true;

		presw = sw;
	}

	if (tsw == LOW) {
		lm.led(B010);
		disp(num[0], num[0]);
		swps = false;
		noTone(BZ_PIN);
		force = false;

	} else {
		if (!force) {
			lm.led(B101);

			if (swps) {
				swps = false;
				move += 15;
				if (++cnt > 7) cnt = 0;
			}

			if (tc >= 10) {
				tc = 0;
				if (move > 0) {
					move--;
					lm.spm(CW);
				}
			}

			if (ph == HIGH) {
				move = 0;
				force = true;
				tone(BZ_PIN, 2000);
			}

		} else {
			lm.led(B001);
		}

		disp(num[cnt / 10], num[cnt % 10]);
	}

	lm.flush();
}
