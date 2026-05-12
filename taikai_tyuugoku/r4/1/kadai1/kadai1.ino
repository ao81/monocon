// 10:41

#define USE_TIMER3_ISR
#include "mono_con.h"

const int hz[2] = { 800, 1600 };

int tsw, pretsw;
int sqsw, lastsw;
bool swps = false;
bool pend = false;
bool busy = false;

int cnt = 0;
int tgl = 1;
int i = 0;

word in, tc, bz;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);

		if (tsw != pretsw) swps = true;

		pretsw = tsw;
	}

	if (bz && --bz == 0) noTone(BZ_PIN);
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (swps) {
		swps = false;

		if (busy) {
			lastsw = tsw;
			pend = true;
		} else {
			cnt++;
			sqsw = tsw;
			tgl = 1;
			i = 0;
			busy = true;
		}
	}

	if (tc >= 100) {
		tc = 0;

		if (busy) {
			if (i < cnt) {
				if (tgl) {
					tone(BZ_PIN, hz[cnt % 2]);
					bz = 100;
					i++;
				}

				tgl = !tgl;
			} else {
				if (pend) {
					pend = false;
					if (lastsw != sqsw) cnt++;
					sqsw = lastsw;
					tgl = 1;
					i = 0;
				} else {
					busy = false;
				}
			}
		}
	}
}
