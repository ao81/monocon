#define USE_TIMER3_ISR
#include "mono_con.h"

#define l lm.color.GBR

const int led[2] = { 0x49, 0x7f };
int i = 0;

int tsw, sw;
int presw;
bool swps = false;

int phase = 0, angle = 0;

word in, cd, tc;

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

typedef enum {
	INIT,
	CD,
	MOVE,
	RES,
	END,
	WAIT,
} status;
status st = INIT, prest;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (st == MOVE && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (cd > 0) cd--;

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw == LOW) st = INIT;

	switch (st) {
	case INIT:
		if (st != prest) prest = st;

		if (tsw == HIGH) st = CD;
		disp(0x00, 0x00);

		break;
	case CD:
		if (st != prest) {
			prest = st;
			cd = 3000;
		}

		if (cd > 0) {
			int t = (cd + 999) / 1000;
			disp(0x00, num[t]);
		} else {
			disp(0x00, 0x00);
			st = MOVE;
		}

		break;
	case MOVE:
		if (st != prest) {
			prest = st;
			tc = 30;
			angle = 0;
		}

		if (tc >= 30) {
			tc = 0;
			cw();
			angle++;
		}

		if (angle >= 30 || swps) {
			swps = false;
			st = RES;
		}

		break;
	case RES:
		static bool ok = true;
		if (st != prest) {
			prest = st;
			ok = (angle >= 10 && angle <= 20);

			if (ok) {
				st = END;
				break;
			} else {
				cd = 1000;
			}
		}

		if (cd == 0) {
			if (angle != 30) {
				if (tc > 20) {
					tc = 0;
					cw();
					angle++;
				}
			} else {
				st = END;
				break;
			}
		}

		break;
	case END:
		if (st != prest) {
			prest = st;
			tc = 500;
			i = 0;
		}

		if (tc >= 500) {
			tc = 0;
			if (i % 2 == 0) disp(led[(int)ok], led[(int)ok]);
			else disp(0x00, 0x00);
			if (++i >= 6) st = WAIT;
		}

		break;
	case WAIT:
		break;
	}
}
