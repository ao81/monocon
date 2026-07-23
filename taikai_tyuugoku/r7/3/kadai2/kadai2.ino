#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw;
int presw;
bool swps = false;
word tc;

int n = 20;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);

	randomSeed(analogRead(A0));
}

void loop() {
	if (swps) {
		swps = false;
		tc = 500;
	}

	if (!tsw) { // off
		if (sw == LOW) {
			if (tc >= 500) {
				tc = 0;
				n = random(0, 100);
			}
		}

		if (n > 99) {
			int m = n % 100;
			disp(num[m / 10], num[m % 10]);
		} else disp(num[n / 10], num[n % 10]);
	} else { // on
		if (sw == LOW) {
			if (tc >= 500) {
				tc = 0;
				n = random(0, 256);
			}
		}

		disp(num[n / 16], num[n % 16]);
	}
}
