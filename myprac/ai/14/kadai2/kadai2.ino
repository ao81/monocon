#define USE_TIMER3
#include "mono_con.h"

const int led[5] = { B100, B100, B101, B001, B001 };

int x, y, tsw, sw, ph;
int presw, preph;
bool swps = false, phdps = false, phups = false;
bool r = true;
uint_fast16_t tc = 0;

int phase = 0;
bool move = false;

int n;
int mx = 0;

word cd = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}

	if (move) tc++;
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;
		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == HIGH && preph == LOW) phdps = true;
		if (ph == LOW && preph == HIGH) phups = true;

		presw = sw;
		preph = ph;
	}

	if (tsw == LOW) {
		if (phase == 0) {
			move = false;
			tc = 0;
			if (phdps) {
				phdps = false;
				phase++;
				move = true;
			}
		} else if (phase == 1) {
			if (phups) {
				phups = false;
				phase++;
			}
		} else if (phase == 2) {
			if (phdps) {
				phdps = false;
				move = false;

				if (tc >= 2000) {
					n = 1;
				} else if (tc >= 1200) {
					n = 2;
				} else if (tc >= 700) {
					n = 3;
				} else if (tc >= 300) {
					n = 4;
				} else {
					n = 5;
					tone(BZ_PIN, 2000, 1000);
				}

				if (n > mx) mx = n;

				phase++;
			}
		} else {
			if (phups) {
				phups = false;
				phase = 0;
			}
		}

		if (swps) {
			swps = false;
			cd = 1000;
		}

		if (cd > 0) disp(0x00, num[mx]);
		else disp(0x00, num[n]);

		lm.led(n == 0 ? 0x00 : led[n - 1]);

	} else {
		disp(0x00, num[0]);
		phase = 0;
		n = mx = 0;
		lm.led(B000);
	}

	lm.flush();
}
