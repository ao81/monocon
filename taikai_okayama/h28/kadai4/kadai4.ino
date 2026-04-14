// 16:21 phの処理が未実装

// js左,右をsw1,2とする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int lo = 0 + 200;
const int hi = 1023 - 200;

int Lcnt = 0, Rcnt = 0;
int xpos, sw, tsw, ph;
int prexpos, presw, preph;

word in;

void onedisp(int n, int m) {
	static int pren = 0, prem = 0;
	if (n != pren || m != prem) {
		pren = n; prem = m;
		disp(num[n % 16], num[m % 16]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		xpos = analogRead(A1);
		sw = digitalRead(_USER_CON_4PIN);
		tsw = digitalRead(_USER_CON_5PIN);
		ph = digitalRead(_USER_CON_3PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	xpos = prexpos = analogRead(A1);
	sw = presw = digitalRead(_USER_CON_4PIN);
	tsw = digitalRead(_USER_CON_5PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);

	disp(num[0], num[0]);
}

void loop() {
	if (xpos > hi && prexpos <= hi) { // sw1
		Lcnt += (tsw == HIGH ? -1 : 1);
		if (Lcnt < 0) Lcnt = 0;
		if (Lcnt > 15) Lcnt = 15;
	}

	if (xpos < lo && prexpos >= lo) { // sw2
		Rcnt += (tsw == HIGH ? -1 : 1);
		if (Rcnt < 0) Rcnt = 0;
		if (Rcnt > 15) Rcnt = 15;
	}

	if (sw == LOW && presw == HIGH) { // sw
		Lcnt = Rcnt = 0;
	}


	prexpos = xpos;
	presw = sw;

	onedisp(Lcnt, Rcnt);
}