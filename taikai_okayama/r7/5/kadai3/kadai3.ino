#define USE_TIMER3_ISR
#include "mono_con.h"

const int spd[2] = { 70, 140 };
int tsw, y;

word in, tc;

void cw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

int getidx(int n) {
	return constrain(map(n, 0, 1023, 0, 3), 0, 2);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		y = analogRead(A2);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	y = analogRead(A2);
}

void loop() {
	int idx = getidx(y);
	int n = 0;

	if (tsw == HIGH) {
		if (idx == 0) {
			cw(spd[1]);
			n = 2;
		} else if (idx == 2) {
			ccw(spd[1]);
			n = -2;
		} else stop();
	} else {
		if (idx == 0) {
			cw(spd[0]);
			n = 1;
		} else if (idx == 2) {
			ccw(spd[0]);
			n = -1;
		} else stop();
	}

	disp(num[abs(n)], (n < 0) ? 0x40 : 0x00);
}

