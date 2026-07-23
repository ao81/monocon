// 4:43

#define USE_TIMER3_ISR
#include "mono_con.h"

const int bzpara[2] = { 2000, 500 };
int bzidx = 0;

int tsw, sw;
int pretsw;
int cnt = 0;

word in, tc;

void onedisp(int n) {
	static int pren = -3;
	if (pren != n) {
		pren = n;
		if (n < 0) disp(num[abs(n)], 0x40);
		else disp(num[n], 0x00);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			bzidx = 0;
			tc = 1000;
		}
		if (tc >= 1000) {
			tc = 0;
			tone(BZ_PIN, bzpara[bzidx]);
			bzidx = !bzidx;
			cnt++;
		}
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			cnt = 0;
			noTone(BZ_PIN);
		}
	}

	onedisp(cnt);
}
