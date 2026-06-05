#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool r = true;

int mx = 0;
word cd = 0;

int getidx(int yy) {
	yy = constrain(yy, 512, 1023);
	return constrain(map(yy, 512, 1023, 0, 10), 0, 9);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin4);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	if (tsw == LOW) {
		int i = getidx(y);

		if (i == 0) {
			lm.led(B000);
			noTone(BZ_PIN);
		} else if (i <= 3) {
			lm.led(B100);
			noTone(BZ_PIN);
		} else if (i <= 6) {
			lm.led(B101);
			noTone(BZ_PIN);
		} else {
			lm.led(B001);
			tone(BZ_PIN, 2000);
		}

		if (mx < i) mx = i;
	}

	if (swps) {
		swps = false;
		cd = 1000;
		disp(0x00, num[mx]);
		mx = 0;
	}

	if (cd <= 0) disp(0x00, 0x00);

	lm.flush();
}
