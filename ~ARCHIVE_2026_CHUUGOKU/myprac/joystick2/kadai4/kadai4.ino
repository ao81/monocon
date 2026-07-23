#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;
int x, y, tsw, sw, ph;
bool r = true;

word tc;

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

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
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
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);
	}

	int i = getdir(x, y);
	if (i == -1) {
		stop();
	} else {
		if (i == 0) {
			cw();
		} else if (i == 1) {
			if (tc >= 20) {
				tc = 0;
				lm.spm(CW).flush();
			}
			stop();
		} else if (i == 2) {
			if (tc >= 20) {
				tc = 0;
				lm.spm(CCW).flush();
			}
			stop();
		} else {
			ccw();
		}
	}
}
