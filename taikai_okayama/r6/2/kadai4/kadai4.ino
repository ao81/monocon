// 17:42
// sw2 => ph

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
int ondo = 20;
int tgl = 0;

word in, tc;

void onedisp(int n, bool f = false) {
	static int pren = -1;
	if (f) {
		pren = -1;
		disp(0x00, 0x71);
	} else if (pren != n) {
		pren = n;
		if (n >= 100) disp(num[0] | 0x80, 0x38);
		else disp(num[n / 10], num[n % 10]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (tsw == HIGH) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == LOW) {
		if (tsw != pretsw) {
			pretsw = tsw;
		}

		if (swps) {
			swps = false;
			if (ondo > 16) ondo--;
		}

		if (phps) {
			phps = false;
			if (ondo < 40) ondo++;
		}

		onedisp(ondo);
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			tgl = 0;
			tc = 500;
		}

		if (tc >= 500) {
			tc = 0;
			onedisp(ondo * 9 / 5 + 32, (tgl == 0));
			if (++tgl > 1) tgl = 0;
		}
	}
}
