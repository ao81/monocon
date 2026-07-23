#define USE_TIMER3
#include "mono_con.h"

const int led[3] = { B001, B101, B010 };
const int seg1[4] = { 0x01, 0x06, 0x08, 0x30 };
const int seg2[4] = { 0x01, 0x30, 0x08, 0x06 };

int tsw, sw, ph;
int presw;
bool swps = false;
bool ison;

int idx = 0;

int move = 0, phase = 0;
word tc, cd;

void cw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

typedef enum {
	WAIT,
	N1,
	N2,
	N3,
	N4,
	N5,
} status;
status st = WAIT;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = !digitalRead(pin3);
		sw = digitalRead(pin5);
		ph = digitalRead(pin4);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	tc++;
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin3);
	sw = presw = digitalRead(pin5);
	ph = digitalRead(pin4);
}

void loop() {
	if (swps) {
		swps = false;
		if (st == WAIT) {
			ison = tsw;
			st = N1;
			if (ison) {
				move = 30;
			} else {
				tc = 200;
				idx = 0;
			}
		}
	}

	switch (st) {
	case WAIT:

		break;
	case N1:
		if (ison) {
			if (move > 0) {
				if (tc >= 40) {
					tc = 0;
					lm.spm(CW);
					move--;
				}
			} else {
				st = N2;
				cd = 2000;
			}
		} else {
			if (tc >= 200) {
				tc = 0;
				if (++idx > 5) {
					st = N2;
					cd = 3000;
					lm.led(B111);
					break;
				}
				tone(BZ_PIN, 2000, 100);
			}
		}

		break;
	case N2:
		if (ison) {
			if (cd > 0) {
				cw(80);
			} else {
				stop();
				tc = 1000;
				idx = -1;
				st = N3;
			}
		} else {
			if (cd <= 0) {
				lm.led(B000);
				st = N3;
				idx = -1;
				tc = 1000;
			}
		}

		break;
	case N3:
		if (ison) {
			if (tc >= 1000) {
				tc = 0;
				if (++idx > 3) {
					st = N4;
					disp(0x00, 0x00);
					idx = -1;
					tc = 1000;
					break;
				}
				disp(seg1[idx], 0x00);
			}
		} else {
			if (tc >= 1000) {
				tc = 0;
				if (++idx > 3) {
					st = N4;
					disp(0x00, 0x00);
					cd = 3000;
					ccw(180);
					break;
				}
				disp(0x00, seg2[idx]);
			}
		}

		break;
	case N4:
		if (ison) {
			if (tc >= 1000) {
				tc = 0;
				if (++idx > 2) {
					st = N5;
					lm.led(B000);
					cd = 2000;
					tone(BZ_PIN, 800);
					break;
				}
				lm.led(led[idx]);
			}
		} else {
			if (cd <= 0) {
				stop();
				move = 60;
				st = N5;
			}
		}

		break;
	case N5:
		if (ison) {
			if (cd <= 0) {
				noTone(BZ_PIN);
				st = WAIT;
			}
		} else {
			if (tc >= 40) {
				tc = 0;
				lm.spm(CCW);
				if (--move <= 0) {
					st = WAIT;
				}
			}
		}

		break;
	}

	lm.flush();
}
