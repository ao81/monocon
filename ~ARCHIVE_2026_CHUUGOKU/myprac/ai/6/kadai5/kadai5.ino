#define USE_TIMER3
#include "mono_con.h"

const int speed = 120;
int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;

int angle = 20, move = 0;
word tc, bz;

bool light = false;

int getidx(int yy) {
	return constrain(map(yy, 0, 1023, 0, 3), 0, 2);
}

int getang(int xx) {
	if (xx > 511 - 200 && xx < 511 + 200) return 20;
	return constrain(map(xx, 0, 1023, 0, 41), 0, 40);
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

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
	bz++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin5);
	sw = digitalRead(pin4);
	ph = digitalRead(pin3);
}

void loop() {
	if (tsw == HIGH) {
		int i = getidx(y);

		if (ph == HIGH) {
			if (i == 2) {
				stop();
				lm.led(B001);
			} else if (i == 1) {
				lm.led(B001);
			}
		} else {
			if (i == 0) { ccw(); disp(num[9], num[9]); }
			else if (i == 2) { cw(); disp(num[9], num[9]); }
			else { stop(); disp(num[0], num[0]); }
		}

		move = angle - getang(x);
		if (tc >= 20) {
			tc = 0;
			if (move > 0) {
				lm.spm(CCW);
				move--;
				angle--;
			} else if (move < 0) {
				lm.spm(CW);
				move++;
				angle++;
			}
		}

		if (swps) {
			swps = false;
			light = !light;
		}

		if (light) {
			lm.led(B111);
		} else {
			if (ph == LOW) lm.led(B000);
			if (i == 0) {
				if (bz >= 1000) {
					bz = 0;
					tone(BZ_PIN, 800, 500);
				}
			} else {
				noTone(BZ_PIN);
				bz = 1000;
			}
		}
	} else {

	}
	lm.flush();
}
