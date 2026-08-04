// 5:43

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;

word in;

void cw(int speed = 80) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int speed = 80) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
	}
}

void setup() {
	config_init();
	serial_init();
	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == LOW) {
		lm.color.GBR = B100;
		stop();
	} else {
		lm.color.GBR = B001;
		if (sw == LOW) {
			cw();
		} else {
			if (ph == HIGH) {
				ccw();
			} else {
				stop();
			}
		}
	}
	led_stepmotor(lm.b8);
}