#define USE_TIMER3_ISR
#include "mono_con.h"

const int speed = 100;

int tsw, sw, ph;
int pretsw, preph;
bool phps = false;

int cnt = 0;

volatile word in, tc;

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
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (ph == HIGH && preph == LOW) phps = true;

		preph = ph;
	}

	if (tc > 0) tc--;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
		}

		if (sw == LOW) {
			cw();
		} else {
			ccw();
		}

		if (phps) {
			phps = false;
			if (++cnt > 99) {
				cnt = 0;
				tc = 200;
			}
		}

		if (tc == 0) {
			lm.color.GBR = B010;
		} else {
			lm.color.GBR = B000;
		}

	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			cnt = 0;
			stop();
			lm.color.GBR = B000;
		}
	}

	disp(num[cnt / 10], num[cnt % 10]);
	led_stepmotor(lm.b8);
}