// 26:36

#define USE_TIMER3_ISR
#include "mono_con.h"

const int blank = 0x00;
const int segParam1[4] = { 0x01, 0x06, 0x08, 0x30 };
const int segParam2[4] = { 0x01, 0x30, 0x08, 0x06 };
const int ledParam[3] = { B001, B101, B010 };

int sw, tsw;
int presw;
bool swps = false;
int phase = 1;
int cnt = 0, ph = 0;

word in, tc;

typedef enum {
	WAIT,
	MOVE_OFF,
	MOVE_ON
} Status;
Status st = WAIT;

void DMcw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_5PIN);
		tsw = digitalRead(_USER_CON_4PIN);

		if (st == WAIT && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	switch (st) {
	case WAIT:
		if (swps) {
			swps = false;
			st = (tsw ? MOVE_ON : MOVE_OFF);
			phase = 1;
			cnt = 0;
			if (st == MOVE_ON) tc = 201;
			else tc = 21;
		}

		break;
	case MOVE_OFF:
		if (phase == 1) {
			if (cnt <= 30) {
				if (tc > 20) {
					tc = 0;
					lm.color.SM = stepm_init(ph);
					led_stepmotor(lm.b8);
					if (++ph > 3) ph = 0;
					cnt++;
				}
			} else {
				lm.color.SM = stepm_init(4);
				led_stepmotor(lm.b8);
				phase = 2;
				tc = 0;
				DMcw(80);
			}
		} else if (phase == 2) {
			if (tc > 2000) {
				DMstop();
				phase = 3;
				tc = 1001;
				cnt = 0;
			}
		} else if (phase == 3) {
			if (cnt <= 4) {
				if (tc > 1000) {
					tc = 0;
					disp(0x00, segParam1[cnt]);
					cnt++;
				}
			} else {
				disp(blank, blank);
				tc = 1001;
				cnt = 0;
				phase = 4;
			}
		} else if (phase == 4) {
			if (cnt <= 3) {
				if (tc > 1000) {
					tc = 0;
					lm.color.GBR = ledParam[cnt];
					led_stepmotor(lm.b8);
					cnt++;
				}
			} else {
				lm.color.GBR = B000;
				led_stepmotor(lm.b8);
				tc = 0;
				phase = 5;
				tone(BZ_PIN, 800);
			}
		} else if (phase == 5) {
			if (tc > 2000) {
				noTone(BZ_PIN);
				st = WAIT;
			}
		}

		break;
	case MOVE_ON:
		if (phase == 1) {
			if (cnt <= 10) {
				if (tc > 200) {
					tc = 0;
					if (cnt % 2) tone(BZ_PIN, 1500);
					else noTone(BZ_PIN);
					cnt++;
				}
			} else {
				tc = 0;
				lm.color.GBR = B111;
				led_stepmotor(lm.b8);
				phase = 2;
			}
		} else if (phase == 2) {
			if (tc > 3000) {
				lm.color.GBR = B000;
				led_stepmotor(lm.b8);
				phase = 3;
				cnt = 0;
				tc = 1001;
			}
		} else if (phase == 3) {
			if (cnt <= 4) {
				if (tc > 1000) {
					tc = 0;
					disp(0x00, segParam2[cnt]);
					cnt++;
				}
			} else {
				disp(blank, blank);
				tc = 0;
				DMccw(180);
				phase = 4;
			}
		} else if (phase == 4) {
			if (tc > 3000) {
				DMstop();
				cnt = 0;
				tc = 21;
				phase = 5;
			}
		} else if (phase == 5) {
			if (cnt <= 60) {
				if (tc > 20) {
					tc = 0;
					lm.color.SM = stepm_init(ph);
					led_stepmotor(lm.b8);
					if (--ph < 0) ph = 3;
					cnt++;
				}
			} else {
				lm.color.SM = stepm_init(4);
				led_stepmotor(lm.b8);
				st = WAIT;
			}
		}

		break;
	}
}