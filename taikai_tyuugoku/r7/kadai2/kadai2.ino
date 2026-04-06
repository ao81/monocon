#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, pretsw;
int randomVal = 0;

word in, rnd;

ISR(TIMER3_COMPA_vect) {
	if (++in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw = digitalRead(_USER_CON_5PIN);
	}

	rnd++;
}

int getRandomVal(int type) {
	if (type == 0) { // 0~99
		return random(100);
	} else { // 0~255
		return random(256);
	}
}

void setup() {
	config_init();
	serial_init();
	randomSeed(millis());

	disp(num[0], num[0]);

	tsw = pretsw = digitalRead(_USER_CON_4PIN);
	sw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (sw == LOW) {
		if (rnd > 500) {
			rnd = 0;
			randomVal = getRandomVal(tsw);
			if (tsw == LOW) {
				int val10 = randomVal % 100;
				disp(num[val10 / 10], num[val10 % 10]);
			} else {
				disp(num[randomVal / 16], num[randomVal % 16]);
			}
		}
	} else {
		rnd = 500;
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == LOW) {
			int val10 = randomVal % 100;
			disp(num[val10 / 10], num[val10 % 10]);
		} else {
			disp(num[randomVal / 16], num[randomVal % 16]);
		}
	}
}