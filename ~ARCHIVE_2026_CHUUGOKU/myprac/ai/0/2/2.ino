// 途中まで

#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;
int x, y;
int tsw, sw, ph;
int pretsw, presw;
bool swps = false, phps = false;
int phase = 0;
word tc;

int getidx(int xx, int yy) {
	int dz = 200;
	int dx = xx - 512;
	int dy = yy - 512;

	if (abs(dx) < dz && abs(dy) < dz) return 0;
	if (abs(dx) > abs(dy)) {
		if (dx > 0) return 3;
		else return 4;
	} else {
		if (dy > 0) return 1;
		else return 2;
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

typedef enum {
	syudou,
	makiage,
	idou,
	makisage,
	kenti,
	kikan,
	countup,
	kinkyu,
} status;
status st;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(A2);
		y = analogRead(A1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);
	tsw = pretsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = digitalRead(pin3);

	if (tsw == LOW) st = syudou;
	else st = makiage;
}

void loop() {
	if (tsw == LOW) {
		lm.led(B100);
		st = syudou;
		swps = false;
	} else {
		lm.led(B010);

		if (swps) {
			swps = false;
			// TODO: 緊急停止モード
		}
	}

	if st !=

	switch (st) {
	case syudou:
		switch (getidx(x, y)) {
		case 0:
			stop();

			break;
		case 1:
			cw();

			break;
		case 2:
			ccw();

			break;
		case 3:
			stop();

			if (tc >= 30) {
				tc = 0;
				if (--phase < 0) phase = 3;
				lm.spm(phase);
			}

			break;
		case 4:
			stop();

			if (tc >= 30) {
				tc = 0;
				if (++phase > 3) phase = 0;
				lm.spm(phase);
			}

			break;
		}

		break;
	case makiage:
		cw();
		if ()

		break;
	case idou:


		break;
	case makisage:


		break;
	case kenti:


		break;
	case kikan:


		break;
	case countup:


		break;
	case kinkyu:


		break;
	}

	lm.flush();
}
