#define USE_TIMER3_ISR
#include "mono_con.h"

const int bz[2] = { 1600, 800 };
int idx = 0;
int cnt = 0;

int tsw, sw;
int pretsw, presw;
bool swps = false;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (tsw == LOW && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);

	pretsw = !tsw;
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			idx = 0;
			tc = 1000;
		}

		if (tc >= 1000) {
			tc = 0;
			tone(BZ_PIN, bz[idx]);
			if (++idx > 1) idx = 0;
			if (++cnt > 99) cnt = 0;
		}

	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			noTone(BZ_PIN);
		}

		if (swps) {
			swps = false;
			cnt = 0;
		}
	}

	disp(num[cnt / 10], num[cnt % 10]);
}


