// 4:16

// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"

int sw, ph;

int in = 0;

void cw() {
	analogWrite(FIN_PIN, 80);
	digitalWrite(RIN_PIN, LOW);
}

void ccw() {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, 80);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (sw == LOW && ph == HIGH) stop();
	else if (sw == LOW) ccw();
	else if (ph == HIGH) cw();
	else stop();
}