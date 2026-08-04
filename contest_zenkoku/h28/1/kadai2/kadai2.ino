#define USE_TIMER3
#include "mono_con.h"

const int angles[12] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110 };

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false;
bool r = true;

int angle = 0, move = 0;
int n = 0;

word tc = 0;

int getmove(int from, int to) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
}

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

	tsw = pretsw =!digitalRead(pin3);
	sw = presw = !digitalRead(pin4);
	ph = preph = digitalRead(pin5);

	disp(num[1], num[2]);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = !digitalRead(pin3);
		sw = !digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (swps) {
		swps = false;

		if (!tsw) {
			if (++n > 11) n = 0;
		} else {
			if (--n < 0) n = 11;
		}

		move = getmove(angle, angles[n]);

		if (n == 0) disp(num[1], num[2]);
		else disp(num[n / 10], num[n % 10]);
	}

	if (tc >= 50) {
		tc = 0;
		if (move > 0) {
			move--;
			angle++;
			lm.spm(CW);
		} else if (move < 0) {
			move++;
			angle--;
			lm.spm(CCW);
		}
		angle = (angle + 120) % 120;
	}

	if (move != 0) lm.led(B111);
	else lm.led(B000);

	lm.flush();
}
