#define USE_TIMER3
#include "mono_con.h"

const int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

int move = 0;

int mon = 1, day = 1;
word tc = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;
		x = analogRead(A2);
		y = analogRead(A1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (swps) {
		swps = false;
		if (++day > days[mon - 1]) {
			day = 1;
			if (++mon > 12) mon = 1;
			move += 10;
		}
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == HIGH) {
			if (++mon > 12) mon = 1;
			move += 10;
			day = 1;
		}
	}

	if (tc >= 7) {
		tc = 0;
		if (move > 0) {
			move--;
			lm.spm(CW).flush();
		}
	}

	if (day < 10) disp(0x00, num[day]);
	else disp(num[day / 10], num[day % 10]);
}
