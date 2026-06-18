#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;

int tsw, sw;
int presw;
bool swps = false;

int mode = 0;
int cnt = 0;

int move = 0, phase = 0;
bool tgl = true;

word tc, cd, wait;

typedef enum {
	WAIT,
	M1,
	M2,
	M3,
	END,
} status;
status st = WAIT;

void cw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;

	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
	if (cd > 0) cd--;
	if (wait > 0) wait--;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);

	disp(num[0], 0x00);
}

void loop() {
	if (tsw == HIGH && st == WAIT) {
		st = (status)(mode + 1);
		if (mode == 0) {
			cd = 2000;
			cw();
			cnt = 4;
		} else if (mode == 1) {
			tc = 30;
			move = 30;
			cnt = 3;
			cd = 200;
			tgl = true;
		} else {
			cnt = 10;
			tc = 300;
		}
	}
	if (st != WAIT) swps = false;

	switch (st) {
	case WAIT:
		if (swps) {
			swps = false;
			if (++mode > 2) mode = 0;
			disp(num[mode], 0x00);
		}

		break;
	case M1:
		if (cd <= 0) stop();
		if (cnt > 0) {
			if (tc >= 1000) {
				tc = 0;
				tone(BZ_PIN, 2000, 500);
				cnt--;
			}
		} else {
			st = END;
		}

		break;
	case M2:
		if (cnt > 0) {
			if (wait <= 0) {
				if (move > 0) {
					if (tc >= 40) {
						tc = 0;
						lm.spm(CW);
						if (--move <= 0) {
							if (--cnt == 1) move = 60;
							else move = 30;
							wait = 500;
						}
					}
				}
			}

			if (cd <= 0) {
				cd = 200;
				if (wait > 0) lm.led(B000);
				else if (tgl) lm.led(B111);
				else lm.led(B000);
				tgl = !tgl;
			}
		} else {
			lm.led(B000);
			st = END;
		}

		break;
	case M3:
		if (cnt > 0) {
			if (tc >= 300) {
				tc = 0;
				disp(0x00, num[--cnt]);
			}
		} else {
			tone(BZ_PIN, 2000, 1000);
			cd = 1000;
			st = END;
		}

		break;
	case END:
		if (cd <= 0 && tsw == LOW) {
			disp(num[mode], 0x00);
			st = WAIT;
		}
		break;
	}

	lm.flush();
}
