#define USE_TIMER3_ISR
#include "mono_con.h"

int SW1 = HIGH, SW2 = HIGH;

word in;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		SW1 = digitalRead(19);
		SW2 = digitalRead(17);
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
	if (SW1 == LOW && SW2 == HIGH) {
		DMccw(100);
	} else if (SW1 == HIGH && SW2 == LOW) {
		DMcw(100);
	} else {
		DMstop();
	}
}