#define USE_TIMER3
#include "mono_con.h"

const int seg1[4] = { 0x01, 0x06, 0x08, 0x30 };

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

		tsw = digitalRead(pin3);
		sw = digitalRead(pin5);
		ph = digitalRead(pin4);

		if (sw == LOW && presw == HIGH) swps = true;

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
		}

		break;
	case N2:
		if (ison) {
			if (cd > 0) {
				cw(80);
			} else {
				stop();
				tc = 1000;
				idx = 0;
				st = N3;
			}
		}

		break;
	case N3:
		if (ison) {
			if (tc >= 1000) {
				tc = 0;

			}
		}

		break;
	case N4:


		break;
	case N5:


		break;
	}

	lm.flush();
}
