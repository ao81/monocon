#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
bool r = true;

int agl = 0, mv = 0;
word tc;

int getmove(int from, int to) {
	return to - from;
}

int getdir(int xx) {
	return constrain(map(xx, 0, 1023, 0, 60), 0, 59);
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
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);
	}

	int i = getdir(x);
	if (abs(i - agl) > 2) {
		mv = getmove(agl, i);
	}

	if (tc >= 7) {
		tc = 0;

		if (mv > 0) {
			lm.spm(CW);
			mv--;
			agl++;
		} else if (mv < 0) {
			lm.spm(CCW);
			mv++;
			agl--;
		}
		lm.flush();
	}
}
