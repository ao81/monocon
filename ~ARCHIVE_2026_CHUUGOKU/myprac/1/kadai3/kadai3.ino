// 21:48

#define USE_TIMER3_ISR
#include "mono_con.h"

const int speed[3] = { 70, 100, 130 };
const int edges[7] = { 100, 200, 300, 700, 800, 900, 1023 };
int idx = 3;

int x, y;

word in;

int getidx(int x_, int y_) {
	int n;
	int cx = abs(512 - x_);
	int cy = abs(512 - y_);
	if (cx > cy) n = x_;
	else n = y_;

	for (int i = 0; i < 7; i++) {
		if (n < edges[i]) return i;
	}
}

void cw(int spd) {
	analogWrite(FIN_PIN, spd);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int spd) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, spd);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		x = analogRead(A1);
		y = analogRead(A2);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	y = analogRead(A2);
}

void loop() {
	idx = getidx(x, y);

	switch (idx) {
	case 0:
		cw(speed[2]);
		break;
	case 1:
		cw(speed[1]);
		break;
	case 2:
		cw(speed[0]);
		break;
	case 3:
		stop();
		break;
	case 4:
		ccw(speed[0]);
		break;
	case 5:
		ccw(speed[1]);
		break;
	case 6:
		ccw(speed[2]);
		break;
	}
}