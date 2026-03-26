#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledParam[3] = { B001, B100, B010 };

int led_idx = 0;

const int D1 = 18; // TSW
const int D2 = 19; // SW1
const int D3 = 17; // SW2

int TSW = 0, SW1 = 0, SW2 = 0;
int prev_SW1 = 0, prev_TSW = 0;

word in;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		prev_SW1 = SW1;
		prev_TSW = TSW;

		TSW = digitalRead(D1);
		SW1 = digitalRead(D2);
		SW2 = digitalRead(D3);
	}
	in++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (TSW == HIGH) {
		if (SW1 == LOW) {
			lm.color.GBR = B001;
		} else if (SW2 == LOW) {
			lm.color.GBR = B100;
		} else  {
			lm.color.GBR = B000;
		}

	} else {
		if (TSW != prev_TSW) {
			led_idx = 0;
			lm.color.GBR = ledParam[led_idx++];
		}

		if (SW1 == LOW && prev_SW1 == HIGH) {
			lm.color.GBR = ledParam[led_idx];
			if (++led_idx > 2) led_idx = 0;
		}
	}

	led_stepmotor(lm.b8);
}