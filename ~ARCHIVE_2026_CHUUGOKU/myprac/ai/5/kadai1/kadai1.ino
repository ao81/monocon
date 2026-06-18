#define USE_TIMER3
#include "mono_con.h"

const int led[3] = { B001, B100, B010 };

int sw, ph;
int presw, preph;
bool swps = false;

bool mode;
int cnt = 0;
int phase = 0, angle;
int idx = 0;

word tc, cd;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(pin_4);
		ph = digitalRead(pin_3);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin_4);
	ph = digitalRead(pin_3);
}

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
	n1,
	n2,
	n3,
	n4,
	n5,
	END,
} status;
status st = WAIT;

void loop() {
	switch (st) {
	case WAIT:
		if (swps) {
			swps = false;
			mode = preph = (ph == LOW ? true : false);
			if (mode) {
				cd = 2000;
			} else {
				cd = 3000;
			}
			st = n1;
		}

		break;
	case n1:
		if (mode) {
			if (cd != 0) {
				lm.led(B001);
			} else {
				lm.led(B000);
				cnt = 0;
				tc = 150;
				st = n2;
			}
		} else {
			if (cd > 0) ccw(160);
			else {
				stop();
				cnt = 3;
				tc = 1000;
				st = n2;
			}
		}

		break;
	case n2:
		if (mode) {
			if (tc >= 200) {
				tc = 0;
				tone(BZ_PIN, 2000, 80);
				cnt++;
			}

			if (cnt > 3) {
				noTone(BZ_PIN);
				angle = 15;
				st = n3;
			}

		} else {
			if (tc >= 1000) {
				tc = 0;
				if (cnt >= 0) {
					disp(num[cnt--], 0x00);
				} else {
					disp(0x00, 0x00);
					cd = 2000;
					st = n3;
				}
			}
		}

		break;
	case n3:
		if (mode) {
			if (tc >= 30) {
				tc = 0;
				if (--phase < 0) phase = 3;
				lm.spm(phase);
				angle--;
			}

			if (angle <= 0) {
				cnt = 0;
				tc = 1000;
				st = n4;
			}
		} else {
			if (cd > 0) tone(BZ_PIN, 800);
			else {
				noTone(BZ_PIN);
				idx = 0;
				tc = 1000;
				st = n4;
			}
		}

		break;
	case n4:
		if (mode) {
			if (tc >= 1000) {
				tc = 0;
				disp(0x00, num[cnt]);
				if (++cnt > 4) {
					disp(0x00, 0x00);
					cd = 2000;
					st = n5;
				}
			}
		} else {
			if (tc >= 1000) {
				tc = 0;
				if (idx > 2) {
					angle = 30;
					tc = 30;
					lm.led(B000);
					st = n5;
				} else lm.led(led[idx++]);
			}
		}

		break;
	case n5:
		if (mode) {
			if (cd > 0) cw(80);
			else {
				stop();
				st = END;
			}
		} else {
			if (tc >= 30) {
				tc = 0;
				if (++phase > 3) phase = 0;
				lm.spm(phase);
				angle--;
			}

			if (angle <= 0) {
				st = END;
			}
		}

		break;
	case END:
		if (ph != preph) st = WAIT;

		break;
	}

	if (st != WAIT && swps) swps = false;

	lm.flush();
}
