#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;

int x, y;
int tsw, sw, ph;
int presw, preph;
bool swps = false;
bool input = true;

bool move = false;
bool tgl = true;
word tc = 0;

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
		input = true;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();
	sw = presw = digitalRead(pin4);
}

void loop() {
	if (input) {
		input = false;
		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	if (ph == HIGH) {
		if (ph != preph) {
			preph = ph;
			stop();
			swps = false;
			move = false;
			tc = 250;
		}

		if (tc >= 250) {
			tc = 0;
			if (tgl) lm.led(B111);
			else lm.led(B000);
			lm.flush();
			tgl = !tgl;
		}

	} else {
		if (ph != preph) {
			preph = ph;
			lm.led(B000).flush();
		}

		if (swps) {
			swps = false;
			move = !move;
		}

		if (move) {
			if (tsw == HIGH) {
				cw();
				lm.led(B100);
			} else {
				ccw();
				lm.led(B001);
			}
		} else {
			stop();
			lm.led(B000);
		}
	}
}
