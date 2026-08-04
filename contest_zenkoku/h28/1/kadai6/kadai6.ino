#define USE_TIMER3
#include "mono_con.h"

const int dspds[4] = { -1, 80, 120, 160 };
const int sspds[4] = { -1, 80, 35, 10 };

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false;
bool r = true;

int phase = 0;
word tc = 0;

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
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = !digitalRead(pin3);
	sw = presw = !digitalRead(pin4);
	ph = preph = digitalRead(pin5);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = !digitalRead(pin3);
		sw = !digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (swps) {
		swps = false;
		if (++phase > 3) phase = 0;
	}

	if (!tsw) {
		if (phase == 0) stop();
		else if (ph == LOW) cw(dspds[phase]);
		else ccw(dspds[phase]);
	} else {
		stop();
		if (phase != 0) {
			if (tc >= sspds[phase]) {
				tc = 0;
				if (ph == LOW) lm.spm(CW);
				else lm.spm(CCW);
				lm.flush();
			}
		}
	}
}
