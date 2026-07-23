// 3:12

#define USE_TIMER3_ISR
#include "mono_con.h"

const int spd[5] = { -180, -90, 0, 90, 180 };

int x, tsw;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		tsw = digitalRead(_USER_CON_5PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	tsw = digitalRead(_USER_CON_5PIN);
}

int getidx(int p) {
	return map(p, 0, 1023, 4, -1);
}

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

void loop() {
	int idx = getidx(x);

	if (spd[idx] < 0) {
		ccw(abs(spd[idx]));
		disp(0x40, num[abs(idx - 2)]);
	} else if (spd[idx] > 0) {
		cw(spd[idx]);
		disp(0x00, num[idx - 2]);
	} else {
		stop();
		disp(0x00, num[0]);
	}
}
