// 12:49

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw;
int pretsw;
int value = 0;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw = digitalRead(_USER_CON_5PIN);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	randomSeed(millis());

	tsw = pretsw = digitalRead(_USER_CON_4PIN);
	sw = digitalRead(_USER_CON_5PIN);

	disp(num[0], num[0]);
}

void loop() {
	if (sw == LOW) {
		if (tc > 500) {
			tc = 0;
			if (tsw == HIGH) {
				value = random(256);
				disp(num[value / 16], num[value % 16]);
			} else {
				value = random(100);
				int dispVal = value % 100;
				disp(num[dispVal / 10], num[dispVal % 10]);
			}
		}
	} else tc = 500;

	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == LOW) {
			int val10 = value % 100;
			disp(num[val10 / 10], num[val10 % 10]);
		} else {
			disp(num[value / 16], num[value % 16]);
		}
	}
}