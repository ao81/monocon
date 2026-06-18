#define USE_TIMER3
#include "mono_con.h"

const int angles[8] = { 0, 15, 30, 45, 60, 75, 90, 105 };

int x, y, tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;
bool r = true;

int n = 0;
int agl = 0, mv = 0;

word tc;

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

	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
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
		if (++n > 7) n = 0;
	}

	if (phps) {
		phps = false;
		mv = getmove(agl, angles[n]);
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
		agl = (agl + 120) % 120;
	}

	disp(num[n], 0x00);
	lm.flush();
}
