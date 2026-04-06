#define USE_TIMER3_ISR
#include "mono_con.h"

int SW = HIGH, PH = LOW;

word in;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		SW = digitalRead(18);
		PH = digitalRead(17);
	}

	in++;
}

void DMcw() {
	analogWrite(FIN_PIN, 100);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw() {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, 100);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (SW == LOW) {
		DMcw();
	} else {
		if (PH == HIGH) {
			DMccw();
		} else {
			DMstop();
		}
	}
}