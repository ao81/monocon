// 途中まで

#define USE_TIMER3
#include "mono_con.h"

const int tempo[3] = { 1000, 500, 300 };

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool input = true;

bool move = false;
word tc;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
	}
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

		if (sw == HIGH && presw == LOW) swps = false;

		presw = sw;
	}

	if (swps) {
		swps = false;
		move = !move;
	}
}
