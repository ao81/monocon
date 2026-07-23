// (3)の途中まで

#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;

int x, y;
int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int cnt = 0;

word tc, wait = 2000;

int getidx(int p) {
	return constrain(map(p, 0, 1023, 0, 10), 0, 9);
}

void cw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw() [
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
]

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
		if (ph == HIGH && presw == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
	if (ph == LOW) wait--;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (tsw == LOW) { // OFF
		lm.led(B100);

		int i = getidx(y);

		if (tc >= 30) {
			tc = 0;
			if (i == 4 || i == 5) lm.spm(STOP);
			else if (i <= 3) lm.spm(CCW);
			else lm.spm(CW);
		}

		if (i == 4 || i == 5) i = 0;
		disp(num[i], 0x00);

		swps = phps = false;
	} else {
		lm.led(B010);
		stop();

		if (phps) {
			phps = false;
			if (++cnt > 99) cnt = 0;
		}

		if (swps) {
			swps = false;
			cnt = 0;
		}

		if (ph == LOW)

		disp(num[cnt / 10], num[cnt % 10]);
	}

	lm.flush();
}
