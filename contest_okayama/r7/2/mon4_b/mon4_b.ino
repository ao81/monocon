#define USE_TIMER3_ISR
#include "mono_con.h"

int pitch[2] = { 1500, 500 };
int idx = 0;

int count = 0;

int tsw, sw;
int presw, pretsw, swps = 0;

word in, bz;

void onedisp(int n) {
	static int pren = -1;
	if (pren != n) {
		disp(num[n / 10], num[n % 10]);
		pren = n;
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_5PIN);
		tsw = digitalRead(_USER_CON_4PIN);

		if (tsw == LOW && sw == LOW && presw == HIGH) swps = 1;

		presw = sw;
	}
	if (tsw == HIGH) bz++;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_5PIN);
	tsw = pretsw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			tone(BZ_PIN, pitch[0]);
			bz = 0;
			idx = 1;
			count++;
		}
		if (bz > 1000) {
			bz = 0;
			tone(BZ_PIN, pitch[idx]);
			if (++idx > 1) idx = 0;
			count++;
		}
	} else {
		noTone(BZ_PIN);
		if (tsw != pretsw) pretsw = tsw;
		if (swps) { swps = 0; count = 0; }
	}
	onedisp(count);
}