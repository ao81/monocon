#define USE_TIMER3_ISR
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;

int user = 5;
int steps = 0;
int rd = 5;

int ledcnt = 0;
int segcnt = 0;
int tgl = 0;

int phase = 0;
word tc;

typedef enum {
	WAIT,
	IN,
	START,
	STOP,
	RES,
} status;
status st;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		y = analogRead(A2);
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (st == IN && sw == LOW && presw == HIGH) swps = true;
		if (st == START && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);

	pretsw = !tsw;
}

/*
1 2 3
4 5 6
7 8 9
*/
int getidx(int xx, int yy) {
	int xidx = map(xx, 0, 1023, 2, -1);
	int yidx = map(yy, 0, 1023, 0, 3);
	return xidx + yidx * 3 + 1;
}

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void loop() {
	if (tsw == HIGH) {
		if (pretsw != tsw) {
			lm.color.GBR = B100;
			pretsw = tsw;
			st = IN;
		}
	} else {
		if (pretsw != tsw) {
			lm.color.GBR = B001;
			pretsw = tsw;
			st = WAIT;
		}
	}

	switch (st) {
	case WAIT:
		disp(0x00, 0x00);

		break;
	case IN: {
		int idx = getidx(x, y);
		disp(0x00, num[idx]);

		if (swps) {
			swps = false;
			user = idx;
			st = START;
			steps = 0;
		}

		break;
	}
	case START: {
		if (tc >= 40) {
			tc = 0;
			cw();
			steps++;

			rd = steps % 10;
		}

		disp(num[user], num[rd]);

		if (phps) {
			phps = false;
			rd = steps % 10;
			st = STOP;
		}

		break;
	}
	case STOP:
		disp(num[user], num[rd]);
		tc = 500;
		ledcnt = segcnt = 0;
		tgl = 0;
		st = RES;

		break;
	case RES:
		if (tc >= 500) {
			tc = 0;
			if (user == rd) {
				if (tgl == 0) {
					if (segcnt++ < 5) disp(0xff, 0xff);
					if (ledcnt++ < 2) lm.color.GBR = B010;
				} else {
					disp(0x00, 0x00);
					lm.color.GBR = B000;
				}

				tgl = !tgl;
			} else {
				lm.color.GBR = B000;
			}
		}

		break;
	}

	led_stepmotor(lm.b8);
}
