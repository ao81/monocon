// 13:09
// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledpara[3][6] = {
	{ 1, 1, 1, 0, 0, 0 },
	{ 0, 1, 1, 1, 0, 0 },
	{ 0, 0, 1, 1, 1, 0 },
};
int idx = 0;

int tsw, sw, ph;
int presw;
bool swps = false;
bool moving = false;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (!moving && tsw == LOW && sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}

	if (tsw == LOW) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	int led = B000;
	if (tsw == HIGH) {
		if (sw == LOW) led |= B001;
		if (ph == HIGH) led |= B100;
		if (sw == LOW && ph == HIGH) led |= B010;
		lm.color.GBR = led;
	} else {
		if (swps) {
			swps = false;
			moving = true;
			idx = 0;
		}
		if (moving && tc > 200) {
			tc = 0;
			lm.bit.R = ledpara[0][idx];
			lm.bit.G = ledpara[1][idx];
			lm.bit.B = ledpara[2][idx];
			// lm.color.GBR = led;
			if (++idx > 5) moving = false;
		}
	}

	led_stepmotor(lm.b8);
}
