#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw = HIGH, sw1 = HIGH, sw2 = HIGH;
int p_tsw = HIGH, p_sw1 = HIGH, p_sw2 = HIGH;

int ondo = 20;
int mode = 1; // 1:C 0:F
int tgl = 1, p_tgl = 1;

word in, wait;

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		tsw = digitalRead(18);
		sw1 = digitalRead(19);
		sw2 = digitalRead(17);
	}

	if (!mode) {
		if (++wait > 500) {
			wait = 0;
			tgl = !tgl;
		}
	} else {
		wait = 0;
		tgl = 1;
	}

	in++;
}

void dispondo() {
	int ten, one;
	if (mode) { // 摂氏
		ten = ondo / 10;
		one = ondo % 10;
	} else { // 華氏
		int f = ondo * 9 / 5 + 32;
		if (f < 100) {
			ten = f / 10;
			one = f % 10;
		} else {
			disp(0xBF, 0x38);
			return;
		}
	}
	disp(num[ten], num[one]);
}

void setup() {
	config_init();
	serial_init();

	dispondo();
}

void loop() {
	int t = tgl;
	int ts = tsw;

	if (ts != p_tsw) {
		mode = !ts;
		if (mode) dispondo();
		else disp(0x00, 0x71);
	} else if (ts == LOW) {
		if (sw1 == LOW && p_sw1 == HIGH && ondo > 16) {
			ondo--;
			dispondo();
		}
		if (sw2 == LOW && p_sw2 == HIGH && ondo < 40) {
			ondo++;
			dispondo();
		}
	} else {
		if (t != p_tgl) {
			if (t) disp(0x00, 0x71);
			else dispondo();
		}
	}

	p_tsw = ts, p_sw1 = sw1, p_sw2 = sw2;
	p_tgl = t;
}