#define USE_TIMER3_ISR
#include "mono_con.h"

const int pitch[3] = { 1000, 500, 2000 };
int idx = 0;
int tgl = 0;

int tsw, sw;
int presw;
bool swps = false;

word tc, bz;

int cnt = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (tsw == LOW && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (tsw == HIGH) tc++;

	if (bz > 0) {
		bz--;
		if (bz == 0) noTone(BZ_PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);

	disp(0x00, 0x00);
}

void loop() {
	if (tsw == HIGH) {
		if (tc >= 1000) {
			tc = 0;
			tgl = !tgl;
			if (++cnt > 99) cnt = 0;
			if (cnt % 10 == 0) {
				idx = 2;
				bz = 100;
			} else {
				idx = tgl;
			}
			tone(BZ_PIN, pitch[idx]);
		}
	} else {
		noTone(BZ_PIN);

		if (swps) {
			swps = false;
			cnt = 0;
		}
	}

	disp(num[cnt / 10], num[cnt % 10]);
}
