// 6:37

#define USE_TIMER3_ISR
#include "mono_con.h"

int sw, ph;
int preph, presw;
bool swps = false, phps = false;

int cnt = 0;
double time = 0.0;

word in, tc;

typedef enum {
	WAIT,
	MOVE,
	RES,
} Status;
Status st = WAIT;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (st == MOVE) tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (st == WAIT) {
		if (phps) {
			phps = false;
			cnt++;
			st = MOVE;
			tc = 0;
		}

		disp(num[cnt / 10], num[cnt % 10]);
	} else if (st == MOVE) {
		if (phps) {
			phps = false;
			cnt++;
		}

		if (tc >= 100) {
			tc = 0;
			if (time < 9.9) time += 0.1;
		}

		if (cnt >= 20) st = RES;

		disp(num[cnt / 10], num[cnt % 10]);
	} else if (st == RES) {
		int c = time * 10;
		disp(num[c / 10] | 0x80, num[c % 10]);
	}

	if (swps) {
		swps = false;
		cnt = 0;
		time = 0.0;
		st = WAIT;
	}
}
