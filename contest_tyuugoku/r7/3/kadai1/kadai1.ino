#define USE_TIMER3_ISR
#include "mono_con.h"

const int spd[2] = { 80, 200 };
const int seg1[5] = { 0x01, 0x06, 0x08, 0x30, 0x00 };
const int led[3] = { B001, B101, B010 };
const int seg2[5] = { 0x01, 0x30, 0x08, 0x06, 0x00 };

int tsw, sw;
int pretsw, presw;
bool swps = false;
int angle = 0, phase = 0;
int mode = 0;
int idx = 0;
bool tgl = true;
int cnt = 0;

word tc, cd;

typedef enum {
	WAIT,
	num1,
	num2,
	num3,
	num4,
	num5,
	END,
} status;
status st = WAIT;

void scw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void sccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

void dcw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void dccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void dstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;

	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (pretsw != tsw) {
		pretsw = tsw;
		st = WAIT;
	}

	switch (st) {
	case WAIT:
		if (swps) {
			swps = false;

			mode = (int)!tsw;
			angle = 30;
			tc = 100;
			st = num1;
			tgl = true;
			cnt = 0;
		}

		break;
	case num1:
		if (mode) {
			if (angle <= 0) {
				st = num2;
				cd = 2000;
				break;
			} else {
				if (tc >= 30) {
					tc = 0;
					scw();
					angle--;
				}
			}
		} else {
			if (tc >= 100) {
				tc = 0;
				if (tgl) tone(BZ_PIN, 2000);
				else noTone(BZ_PIN);
				tgl = !tgl;
				if (cnt++ > 8) {
					st = num2;
					cd = 3000;
					break;
				}
			}
		}

		break;
	case num2:
		if (mode) {
			if (cd == 0) {
				st = num3;
				dstop();
				idx = 0;
				tc = 1000;
				break;
			} else {
				dcw(spd[0]);
			}
		} else {
			if (cd != 0) lm.color.GBR = B111;
			else {
				lm.color.GBR = B000;
				st = num3;
				idx = 0;
				tc = 1000;
				break;
			}
		}

		break;
	case num3:
		if (mode) {
			if (tc >= 1000) {
				tc = 0;
				disp(0x00, seg1[idx]);
				if (++idx > 4) {
					disp(0x00, 0x00);
					st = num4;
					idx = 0;
					tc = 0;
					lm.color.GBR = led[0];
					break;
				}
			}
		} else {
			if (tc >= 1000) {
				tc = 0;
				disp(seg2[idx], 0x00);
				if (++idx > 4) {
					disp(0x00, 0x00);
					st = num4;
					cd = 3000;
					break;
				}
			}
		}

		break;
	case num4:
		if (mode) {
			if (tc >= 1000) {
				tc = 0;
				if (++idx > 2) {
					lm.color.GBR = B000;
					st = num5;
					cd = 2000;
					break;
				}
				lm.color.GBR = led[idx];
			}
		} else {
			if (cd != 0) dccw(spd[1]);
			else {
				dstop();
				st = num5;
				angle = 60;
				tc = 30;
				break;
			}
		}

		break;
	case num5:
		if (mode) {
			if (cd <= 0) {
				noTone(BZ_PIN);
				st = END;
				break;
			} else {
				tone(BZ_PIN, 800);
			}
		} else {
			if (angle > 0) {
				if (tc >= 30) {
					tc = 0;
					sccw();
					angle--;
				}
			} else {
				st = END;
				break;
			}
		}

		break;
	case END: break;
	}

	led_stepmotor(lm.b8);
}
