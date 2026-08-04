#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw;
int pretsw, presw;
bool swps = false;

int phase = 0;
int angle = 0;
bool ok = false;
int cnt = 0;
int i = 0;

word in, cd, tc;

typedef enum {
	INIT,
	CD,
	MOVE,
	RES,
	END,
} status;
status st;

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_3PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (st == MOVE && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;

	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_3PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw == LOW) {
		if (tsw != pretsw) {
			pretsw = tsw;
			st = INIT;
		}
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			st = CD;
			cd = 3000;
		}
	}

	switch (st) {
	case INIT:
		disp(0x00, 0x00);
		break;
	case CD: {
		if (cd == 0) {
			st = MOVE;
			angle = 0;
			ok = false;
			tc = 50;
			disp(0x00, 0x00);
			break;
		}

		int c = (cd + 999) / 1000;
		disp(0x00, num[c]);
		break;
	}
	case MOVE:
		if (tc >= 50) {
			tc = 0;
			cw();
			angle++;
		}

		if (swps || angle >= 30) {
			swps = false;
			st = RES;
			if (angle >= 12 && angle <= 18) {
				ok = true;
			} else {
				cd = 1000;
			}
		}

		break;
	case RES:
		if (ok) {
			tc = 500;
			st = END;
		} else {
			if (cd == 0) {
				if (angle < 30) {
					if (tc >= 20) {
						tc = 0;
						cw();
						angle++;
					}
				} else {
					st = END;
				}
			}
		}

		if (st == END) {
			tc = 500;
			cnt = 0;
			i = 0;
		}

		break;
	case END:
		if (cnt < 6) {
			if (tc >= 500) {
				tc = 0;
				if (!i) {
					if (ok) disp(num[8], num[8]);
					else disp(0x49, 0x49);
				} else {
					disp(0x00, 0x00);
				}
				cnt++;
				if (++i > 1) i = 0;
			}
		}

		break;
	}
}
