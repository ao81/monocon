#define USE_TIMER3
#include "mono_con.h"

const int angles[12] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110 };

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
int swps = false;
bool r = true;

int angle = 0, move = 0;
int n = 30;

word cd = 1000, tc = 0;;

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

	if (tsw && cd > 0) cd--;
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw =!digitalRead(pin3);
	sw = presw = !digitalRead(pin4);
	ph = preph = digitalRead(pin5);

	disp(num[3], num[0]);

	noInterrupts();
	cd = 1000;
	interrupts();
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

	if (cd <= 0) {
		cd = 1000;
		if (n > 0) n--;
	}

	if (!tsw) {
		if (swps) {
			swps = false;
			move = getmove(angle, angles[n % 10]);
		}
	} else if (swps) swps = false;

	if (tc >= 8) {
		tc = 0;
		if (move > 0) {
			move--;
			angle++;
			lm.spm(CW).flush();
		} else if (move < 0) {
			move++;
			angle--;
			lm.spm(CCW).flush();
		}
		angle = (angle + 120) % 120;
	}

	disp(num[n / 10], num[n % 10]);
}
