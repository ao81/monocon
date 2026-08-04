#define USE_TIMER3
#include "mono_con.h"

int tsw, sw;
int n = 0;

word tc;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin5);
	sw = digitalRead(pin4);

	randomSeed(analogRead(0));
}

void loop() {
	if (tsw == LOW) {
		if (sw == LOW) {
			if (tc >= 500) {
				tc = 0;
				n = random(100);
			}
		} else {
			tc = 500;
		}

		disp(num[n % 100 / 10], num[n % 100 % 10]);
	} else {
		if (sw == LOW) {
			if (tc >= 500) {
				tc = 0;
				n = random(256);
			}
		} else {
			tc = 500;
		}

		disp(num[n / 16], num[n % 16]);
	}
}
