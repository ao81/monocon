#define USE_TIMER3_ISR
#include "mono_con.h"

const int segpara[2] = { 0x7f, 0x49 };

int tsw, sw;
int pretsw, presw;
bool swps = false;
bool first = true;

int angle = 0;
int phase = 0;

bool isok = false;
int blcnt = 0;

int cd = -1;
word in, tc, bl;

typedef enum {
	INIT,
	WAIT,
	CD,
	MOVE,
	RES,
	BLINK,
} Status;
Status st;

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if ((st == WAIT || st == MOVE) && sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}

	if (cd > 0) cd--;
	if (bl > 0) bl--;
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);

	pretsw = -1;
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == HIGH) {
			st = INIT;
		} else {
			st = WAIT;
		}
		first = true;
	}

	switch (st) {
	case INIT:
		if (first) {
			first = false;
			lm.color.GBR = B001;
		}


		break;
	case WAIT:
		if (first) {
			first = false;
			lm.color.GBR = B100;
		}

		if (swps) {
			swps = false;
			st = CD;
			first = true;
		}

		break;
	case CD:
		if (first) {
			first = false;
			cd = 3000;
		}

		if (cd > 0) {
			int s = (cd + 999) / 1000;
			disp(0x00, num[s]);
		} else {
			disp(0x00, num[0]);
			st = MOVE;
			first = true;
		}

		break;
	case MOVE:
		if (first) {
			first = false;
			angle = 0;
			tc = 40;
		}

		if (angle >= 30 || swps) {
			swps = false;
			st = RES;
			first = true;
		}

		if (tc >= 40) {
			tc = 0;
			cw();
			angle++;
		}

		break;
	case RES:
		if (first) {
			first = false;
			isok = (angle * 3 >= 36 && angle * 3 <= 54);
			if (!isok) cd = 1000;
		}

		if (isok) {
			st = BLINK;
			first = true;
		} else {
			if (cd == 0) {
				if (angle >= 30) {
					st = BLINK;
					first = true;
				} else if (tc >= 40) {
					tc = 0;
					cw();
					angle++;
				}
			}
		}

		break;
	case BLINK:
		if (first) {
			first = false;
			blcnt = 0;
			bl = 500;
		}

		if (blcnt < 6 && bl == 0) {
			bl = 500;
			if (blcnt % 2 == 0) {
				if (isok) disp(segpara[0], segpara[0]);
				else disp(segpara[1], segpara[1]);
			} else {
				disp(0x00, 0x00);
			}
			blcnt++;
		}

		break;
	}

	led_stepmotor(lm.b8);
}
