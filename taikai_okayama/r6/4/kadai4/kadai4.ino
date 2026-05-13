#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int ondo = 20;
int i = 0;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_3PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_5PIN);

		if (tsw == LOW && sw == LOW && presw == HIGH) swps = true;
		if (tsw == LOW && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_3PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tc >= 500) {
			tc = 0;
			if (!i) {
				disp(0x71, 0x00);
			} else {
				int n = ondo * 9 / 5 + 32;
				if (n < 100) disp(num[n / 10], num[n % 10]);
				else disp(num[0] | 0x80, 0x38);
			}
			if (++i > 1) i = 0;
		}
	} else {
		i = 0;

		if (swps) {
			swps = false;
			if (ondo > 16) ondo--;
		}

		if (phps) {
			phps = false;
			if (ondo < 40) ondo++;
		}

		disp(num[ondo / 10], num[ondo % 10]);
	}
}
