// 7:13

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw;

const int bzpitch[2] = { 1300, 500 };

int bzidx = 0;
bool first = true;

int count = 0;

word in, tc;

void onedisp(int n) {
	static int pren = -1;
	if (n != pren) {
		pren = n;
		disp(num[n / 10], num[n % 10]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
	}

	if (tsw == HIGH) tc++;
}

void setup() {
	config_init();
	serial_init();
	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (first) {
			first = false;
			bzidx = 0;
			tc = 0;
			count++;
			tone(BZ_PIN, bzpitch[bzidx]);
		}

		if (tc > 1000) {
			tc = 0;
			if (++bzidx > 1) bzidx = 0;
			tone(BZ_PIN, bzpitch[bzidx]);
			if (++count > 99) count = 0;
		}
	} else {
		first = true;
		noTone(BZ_PIN);
		if (sw == LOW) {
			count = 0;
		}
	}

	onedisp(count);
}