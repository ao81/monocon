#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledpara[3] = { B001, B100, B010 };

int tsw, sw, ph;
int presw;
bool swps = false;

int idx = 0;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_3PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_5PIN);

		if (sw == LOW && presw == HIGH && tsw == LOW) swps = true;

		presw = sw;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_3PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_5PIN);
}

void loop() {
	int led = B000;
	if (tsw == HIGH) {
		idx = 0;
		if (sw == LOW && ph == HIGH) led |= B010;
		else {
			if (sw == LOW) led |= B001;
			if (ph == HIGH) led |= B100;
		}
		lm.color.GBR = led;
	} else {
		if (swps) {
			swps = false;
			if (++idx > 2) idx = 0;
		}
		lm.color.GBR = ledpara[idx];
	}
	led_stepmotor(lm.b8);
}
