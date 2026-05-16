#define USE_TIMER3_ISR
#include "mono_con.h"

const int speed = 100;
int y;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		y = analogRead(A2);
	}
}

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

void setup() {
	config_init();
	serial_init();

	y = analogRead(A2);
}

int getidx(int yy) {
	return map(yy, 0, 1023, 0, 3);
}

void loop() {
	int i = getidx(y);
	if (i == 0) {
		cw();
		tone(BZ_PIN, 2000);
		lm.color.GBR = B100;
	} else if (i == 2) {
		ccw();
		tone(BZ_PIN, 2000);
		lm.color.GBR = B001;
	} else {
		stop();
		noTone(BZ_PIN);
		lm.color.GBR = B000;
	}
}
