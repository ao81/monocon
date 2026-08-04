// ロータリーエンコーダを用いるデューティー比の処理は無い

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw1, sw2;
int pretsw, presw1, presw2;
bool sw1ps = false, sw2ps = false;

int rotateidx = 0;

word in;

typedef enum {
	STOP,
	ROTATE,
	PAUSE,
} States;
States st = STOP;

void DMcw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void DMstop() {
	digitalWrite(FIN_PIN, LOW);
	digitalWrite(RIN_PIN, LOW);
}

void DMpause() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void (*rotate[])(int) = { DMcw, DMccw };

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1ps = true;
		if (sw2 == LOW && presw2 == HIGH) sw2ps = true;

		presw1 = sw1;
		presw2 = sw2;
	}

	in++;
}

void setup() {
	config_init();
	serial_init();

	disp(num[0], num[0]);
	DMstop();

	tsw = digitalRead(_USER_CON_4PIN);
	sw1 = digitalRead(_USER_CON_5PIN);
	sw2 = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		rotateidx = !rotateidx;
	}

	switch (st) {
	case STOP:
		DMstop();
		lm.color.GBR = B100;

		if (sw1ps) {
			sw1ps = false;
			st = ROTATE;
		}
		if (sw2ps) sw2ps = false;

		break;
	case ROTATE:
		rotate[rotateidx](80);
		lm.color.GBR = B001;

		if (sw1ps) sw1ps = false;
		if (sw2ps) {
			sw2ps = false;
			st = PAUSE;
		}

		break;
	case PAUSE:
		DMpause();
		lm.color.GBR = B101;

		if (sw1ps) {
			sw1ps = false;
			st = ROTATE;
		}

		if (sw2ps) {
			sw2ps = false;
			st = STOP;
		}

		break;
	}

	led_stepmotor(lm.b8);
}