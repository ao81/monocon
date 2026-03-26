#define USE_TIMER3_ISR
#include "mono_con.h"

const int D1 = 17; // SW2
const int D3 = 19; // SW1

int SW1 = 0, SW2 = 0;

word in;

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

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		SW1 = digitalRead(D3);
		SW2 = digitalRead(D1);
	}

	in++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (SW1 == LOW && SW2 == HIGH) {
		DMccw(100);
	} else if (SW1 == HIGH && SW2 == LOW) {
		DMcw(100);
	} else {
		DMstop();
	}
}