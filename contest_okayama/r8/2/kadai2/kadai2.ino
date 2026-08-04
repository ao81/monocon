#define USE_TIMER3_ISR
#include "mono_con.h"

const int speed = 100;
int tsw, sw, ph;

void cw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw() {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void free() {
	digitalWrite(FIN_PIN, LOW);
	digitalWrite(RIN_PIN, LOW);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
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
		lm.color.GBR = B111;
		noTone(BZ_PIN);
		stop();
	} else {
		if (sw == HIGH) {
			if (ph == LOW) {
				free();
				lm.color.GBR = B000;
			} else {
				cw();
				lm.color.GBR = B010;
			}
			noTone(BZ_PIN);
		} else {
			if (ph == LOW) {
				ccw();
				lm.color.GBR = B100;
				noTone(BZ_PIN);
			} else {
				stop();
				lm.color.GBR = B001;
				tone(BZ_PIN, 1000);
			}
		}
	}

	led_stepmotor(lm.b8);
}
