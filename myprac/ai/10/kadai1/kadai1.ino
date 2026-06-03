#define USE_TIMER3
#include "mono_con.h"

const int led[6] = { B001, B100, B010, B101, B110, B111 };
int tsw, sw, ph;
int presw;
bool swps = false;
int n = 1;
word tc;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = digitalRead(pin3);
}

void loop() {
	if (sw == LOW) {
		lm.led(B000);
		if (tc >= 100) {
			tc = 0;
			if (++n > 6) n = 1;
		}
	} else if (swps) {
		swps = false;
		tone(BZ_PIN, 2000, 50);
		lm.led(led[n - 1]);
	}

	lm.flush();
	disp(0x00, num[n]);
}
