// 10:33

#define USE_TIMER3_ISR
#include "mono_con.h"

const int lo = 0 + 200;
const int hi = 1023 - 200;

int spd = 0;

int tsw, ypos;

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

void onedisp(int n, int m) {
	static int pren = 0x00, prem = 0x00;
	if (n != pren || m != prem) {
		pren = n, prem = m;
		disp(n, m);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
		ypos = analogRead(A2);
	}
}

void setup() {
	config_init();
	serial_init();
	tsw = digitalRead(_USER_CON_5PIN);
	ypos = analogRead(A2);
	disp(num[0], 0x00);
}

void loop() {
	if (tsw == HIGH) {
		if (ypos < lo) {
			cw(130);
			spd = 2;
		} else if (ypos > hi) {
			cw();
			spd = 1;
		} else {
			stop();
			spd = 0;
		}
	} else {
		if (ypos < lo) {
			ccw();
			spd = -1;
		} else if (ypos > hi) {
			ccw(130);
			spd = -2;
		} else {
			stop();
			spd = 0;
		}
	}

	if (spd >= 0) {
		onedisp(num[spd], 0x00);
	} else {
		onedisp(num[abs(spd)], 0x40);
	}
}