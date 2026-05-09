#define USE_TIMER3_ISR
#include "mono_con.h"

int TSW = HIGH, SW1 = HIGH, SW2 = HIGH;
int preTSW = HIGH;
int led = B000, cycle = 0;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		TSW = digitalRead(18);
		SW1 = digitalRead(19);
		SW2 = digitalRead(17);
	}

	in++;

	if (TSW == LOW) tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	led = B000;
	if (TSW == HIGH) {
		if (SW1 == LOW) led |= B001;
		if (SW2 == LOW) led |= B100;
		if (SW1 == LOW && SW2 == LOW) led |= B010;

	} else {
		if (TSW != preTSW) { tc = 0; cycle = 0; }
		if (cycle >= 5) {
			led = B000;
		} else {
			if (tc > 1200) { tc = 0; cycle++; }

			if (tc >= 0 && tc < 600) led |= B001;
			if (tc >= 200 && tc < 800) led |= B100;
			if (tc >= 400 && tc < 1000) led |= B010;
		}
	}

	preTSW = TSW;
	
	lm.color.GBR = led;
	led_stepmotor(lm.b8);
}