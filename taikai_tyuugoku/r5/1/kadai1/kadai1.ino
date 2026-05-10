// 41:54
// task3: sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"
#define l lm.color.GBR

const int led[3] = { B001, B100, B010 };

int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

bool lh = false, hl = false;

int n = 0;

word in, bz, tc;

typedef enum {
	INIT,
	SELECT,
	TASK1,
	TASK2,
	TASK3,
} status;
status st, prest;

typedef enum {
	WAIT,
	KIROKU,
	SAISEI,
} T1;
T1 t1;
int idx = 0;

int kiroku[64];
bool playing = false;

int phase = 0;
int angle = 0;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if ((st == INIT || st == SELECT) && tsw == LOW && sw == LOW && presw == HIGH) swps = true;

		if (tsw == HIGH) {
			if (st == TASK1) {
				if (t1 == KIROKU) {
					if (sw == LOW && presw == HIGH) hl = true;
					if (sw == HIGH && presw == LOW) lh = true;
				} else if (t1 == SAISEI) {
					if (sw == LOW && presw == HIGH) swps = true;
				}
			} else if (st == TASK2) {
				if (ph == HIGH && preph == LOW) phps = true;
			} else if (st == TASK3) {
				if (sw == LOW && presw == HIGH) swps = true;
				if (ph == HIGH && preph == LOW) phps = true;
			}
		}

		presw = sw;
		preph = ph;
	}

	if (bz > 0) {
		bz--;
		if (bz == 0) noTone(BZ_PIN);
	}

	if (tsw == HIGH && (st == TASK1 || st == TASK3)) tc++;
}

void kiroku_init() {
	for (int i = 0; i < 64; i++) kiroku[i] = -1;
}

void cw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);

	disp(0x00, 0x00);
	st = INIT;
}

void loop() {
	if (tsw == LOW && st != INIT) {
		st = SELECT;
		l = B000;
	}

	switch (st) {
	case INIT:
		if (swps) {
			swps = false;
			st = SELECT;
			n = 0;
		}

		disp(0x00, 0x00);

		break;
	case SELECT:
		if (swps) {
			swps = false;
			if (++n > 3) n = 1;
			bz = 100;
			tone(BZ_PIN, 1000);
		}

		if (n >= 1 && n <= 3 && tsw == HIGH) {
			st = (status)(n + 1);
			if (st == TASK1) {
				t1 = WAIT;
			} else if (st == TASK2) {
				idx = 0;
			}
		}

		disp(num[n / 10], num[n % 10]);
		break;
	case TASK1:

		if (t1 == WAIT) {
			if (ph == HIGH && sw == LOW) {
				kiroku_init();
				kiroku[0] = tc = 0;
				t1 = KIROKU;
				idx = 1;
				lh = false, hl = false;
			}
		} else if (t1 == KIROKU) {
			if (hl || lh) {
				lh = false, hl = false;
				kiroku[idx] = tc;
				if (idx < 63) idx++;
			}

			if (ph == LOW) {
				t1 = SAISEI;
			}

			if (sw == LOW) {
				bz = 10;
				tone(BZ_PIN, 1000);
			}
		} else if (t1 == SAISEI) {

			if (swps) {
				swps = false;
				playing = true;
				idx = 0;
				tc = 0;
			}

			if (ph == HIGH) {
				t1 = WAIT;
				noTone(BZ_PIN);
			}

			if (playing) {
				if (idx >= 64 || kiroku[idx] == -1) {
					playing = false;
					noTone(BZ_PIN);
					break;
				}

				if (tc >= kiroku[idx]) {
					if (idx % 2 == 0) {
						tone(BZ_PIN, 1000);
					} else {
						noTone(BZ_PIN);
					}
					idx++;
				}
			}
		}

		break;
	case TASK2:
		if (phps) {
			phps = false;
			if (++idx > 2) idx = 0;
		}

		l = led[idx];

		break;
	case TASK3:

		if (swps) {
			swps = false;
			angle = 30;
		}

		if (phps) {
			phps = false;
			angle = -30;
		}

		if (tc > 30) {
			tc = 0;
			if (angle > 0) {
				cw();
				angle--;
			} else if (angle < 0) {
				ccw();
				angle++;
			}
		}

		break;
	}

	led_stepmotor(lm.b8);
}