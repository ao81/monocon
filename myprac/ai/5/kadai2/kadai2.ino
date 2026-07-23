#define USE_TIMER3
#include "mono_con.h"

int tsw, sw;
int cnt = 0;

word tc;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(pin_5);
		sw = digitalRead(pin_4);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin_5);
	sw = digitalRead(pin_4);
}

void loop() {
	if (sw == LOW) {
		if (tc >= 100) {
			tc = 0;
			if (tsw == LOW) {
				if (++cnt > 99) cnt = 0;
			} else {
				if (--cnt < 0) cnt = 99;
			}
		}
	} else {
		tc = 500;
	}

	if (cnt <= 33) lm.led(B010);
	else if (cnt <= 66) lm.led(B101);
	else lm.led(B001);
	lm.flush();

	disp(num[cnt / 10], num[cnt % 10]);
}
