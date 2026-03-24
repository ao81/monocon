#define USE_TIMER3_ISR
#include "mono_con.h"

const int D1 = 17; // PH
const int D2 = 18; // SW
const int D3 = 19; // TSW

int PH  = 0;
int SW  = 0;
int TSW = 0;

const int ledParam[] = { B100, B001 }; // 緑, 赤

word in;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		PH  = digitalRead(D1);
		SW  = digitalRead(D2);
		TSW = digitalRead(D3);
	}

	in++;
}

void setup() {
	config_init();
	serial_init();
}

void DMcw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void loop() {
	if (TSW == LOW) {
		// on
		lm.color.GBR = ledParam[1];

		if (SW == LOW) {
			DMcw(100);
		} else if (PH == HIGH) {
			DMccw(100);
		} else {
			DMstop();
		}

	} else {
		// off
		lm.color.GBR = ledParam[0];

	}

	led_stepmotor(lm.b8);
}