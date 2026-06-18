#define USE_TIMER3_ISR
#include "mono_con.h"

int y;
int pwm = 0;
word tc;

int getidx(int yy) {
	return map(yy, 0, 1023, 0, 3);
}

void cw(int p) {
	analogWrite(FIN_PIN, p);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int p) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, p);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		y = analogRead(A2);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	y = analogRead(A2);
}

void loop() {
	int i = getidx(y);
	if (i == 0) {
		if (tc >= 100) {
			tc = 0;
			if (pwm <= 250) pwm += 5;
		}
	} else if (i == 2) {
		if (tc >= 100) {
			tc = 0;
			if (pwm >= -250) pwm -= 5;
		}
	} else {
		tc = 100;
	}

	if (pwm == 0) {
		stop();
		lm.color.GBR = B000;
	} else if (pwm < 0) {
		ccw(abs(pwm));
		lm.color.GBR = B001;
	} else {
		cw(pwm);
		lm.color.GBR = B100;
	}

	int d = map(pwm, -255, 255, 0, 99);
	disp(num[d / 10], num[d % 10]);

	led_stepmotor(lm.b8);
}
