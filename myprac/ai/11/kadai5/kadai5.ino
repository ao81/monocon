#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
bool input = true;

int angle = 0, move = 0;

word tc = 0, cd = 0;

int getmove(int from, int to) {
	return to - from;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
	}
	if (cd > 0) cd--;
	tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (input) {
		input = false;
		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);
	}

	if (!tsw && ph == HIGH) cd = 3000;

	if (cd > 0) {
		move = getmove(angle, 30);
	} else {
		move = getmove(angle, 0);
	}

	if (tc >= 20) {
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
	}

	if (angle == 0) lm.led(B000);
	else if (move > 0) lm.led(B100);
	else if (move < 0) lm.led(B001);
	else lm.led(B100);

	lm.flush();
}
