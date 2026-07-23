#define USE_TIMER3
#include "mono_con.h"

int sw, ph;
int presw, preph;
bool swps = false, phps = false;

bool move = false;
int cnt = 0;

word tc = 0, cd = 0;

typedef enum {
	WAIT,
	GAME,
	RES,
} status;
status st = WAIT;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (move) tc++;
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	randomSeed(analogRead(0));
}

void loop() {
	if (st == WAIT) {
		if (swps) {
			swps = false;
			cd = random(1000, 3000 + 1);
			st = GAME;
			phps = false;
			lm.led(B001);
			tc = 0;
		}

	} else if (st == GAME) {

		if (cd <= 0) {
			lm.led(B010);
			move = true;
		}

		if (phps) {
			phps = false;
			move = false;

			if (tc == 0) {
				disp(num[0], num[0]);
				tone(BZ_PIN, 800, 1000);
			} else if (tc < 300) {
				disp(num[9], num[9]);
				tone(BZ_PIN, 2000, 100);
				cnt = 2;
				cd = 200;
			} else if (tc < 600) {
				disp(num[5], num[0]);
				tone(BZ_PIN, 2000, 500);
			} else if (tc < 1000) {
				disp(num[2], num[0]);
				tone(BZ_PIN, 800, 500);
			} else {
				disp(num[0], num[5]);
				tone(BZ_PIN, 800, 300);
				cnt = 1;
				cd = 600;
			}

			st = RES;
		}

	} else {
		if (swps) {
			st = WAIT;
			disp(0x00, 0x00);
			lm.led(B001);
		}

		if (cnt > 0) {
			if (tc < 300) {
				if (cd <= 0) {
					cd = 200;
					cnt--;
					tone(BZ_PIN, 2000, 100);
				}
			} else if (tc > 1000) {
				if (cd <= 0) {
					cd = 600;
					cnt--;
					tone(BZ_PIN, 800, 300);
				}
			}
		}
	}

	lm.flush();
}
