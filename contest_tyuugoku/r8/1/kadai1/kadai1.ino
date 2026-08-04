#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
int swps = false, phps = false;
bool r = true;

int cnt = 0;
bool tgl = true;
word tc = 0, cd = 2000;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
	if (ph == HIGH) {
		if (cd > 0) cd--;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	disp(0x00, num[0]);
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

	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			if (++cnt > 20) cnt = 0;
		}
	} else {
		if (swps) {
			swps = false;
			if (--cnt < 0) cnt = 20;
		}
	}

	if (cnt == 0) {
		lm.led(B000);
		tgl = true;
	} else if (cnt <= 10) {
		if (tc >= 500) {
			tc = 0;
			if (tgl) lm.led(B100);
			else lm.led(B000);
			tgl = !tgl;
		}
	} else if (cnt <= 19) {
		if (tc >= 100) {
			tc = 0;
			if (tgl) lm.led(B001);
			else lm.led(B000);
			tgl = !tgl;
		}
	} else {
		lm.led(B001);
		tgl = true;
	}

	if (cd <= 0) {
		cnt = 0;
		swps = false;
		cd = 2000;
	}

	if (cnt < 10) disp(0x00, num[cnt % 10]);
	else disp(num[cnt / 10], num[cnt % 10]);
	lm.flush();
}
