#define USE_TIMER3_ISR
#include "mono_con.h"

int y_pos = 512;

word in;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;
		y_pos = analogRead(A1);
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
	if (y_pos < 256) {
		DMcw();
	} else if (y_pos > 868) {
		DMccw();
	} else {
		DMstop();
	}
}