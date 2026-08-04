#define USE_TIMER3
#include "mono_con.h"

const int spds[5] = { 0, 70, 100, 150, 200 };
const int tcd[5] = { 100, 150, 200, 300, 400 };
int tidx = 0;

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
int swps = false, phps = false;
bool r = true;

int n = 0;
word tc = 0;

bool work = false;
int idx = 0;
word cd = 0;

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

void free() {
	digitalWrite(FIN_PIN, LOW);
	digitalWrite(RIN_PIN, LOW);
}

typedef enum {
	off,
	wait,
	move,
	// stop,
	// end,
} status;
status st = off;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	if (cd > 0) cd--;
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin5);
	pretsw = !tsw;
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
	randomSeed(analogRead(0));
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == LOW && preph == HIGH) phps = true;

		presw = sw;
		preph = ph;
	}

	if (tsw != pretsw) {
		if (tsw == LOW) {
			st = off;
		} else {
			st = wait;
		}
		pretsw = tsw;
	}

	switch (st) {
	case off:
		free();
		idx = 0;
		phps = swps = false;
		disp(0x00, 0x00);

		break;
	case wait:
		if (swps) {
			swps = false;
			st = move;
			idx = 4;
			work = true;
			tidx = 0;
		}

		break;
	case move:
		cw(spds[idx]);

		if (phps) {
			phps = false;
			if (tidx < 4) tidx++;
			if (--idx <= 0) {
				cd = 1000;
				work = false;
				tidx = 4;
			}
		}

		if (work && tc >= tcd[tidx]) {
			tc = 0;
			n = random(1, 100);
			tone(BZ_PIN, 329, 60);
		}

		if (!work) {
			if (cd <= 0) {
				st = wait;
				free();
				swps = phps = false;
				tone(BZ_PIN, 523, 500);
			} else {
				if (tc >= tcd[tidx]) {
					tc = 0;
					n = random(1, 100);
					tone(BZ_PIN, 329, 60);
				}
			}
		}

		if (n < 10) disp(0x00, num[n % 10]);
		else disp(num[n / 10], num[n % 10]);

		break;
	}
}
