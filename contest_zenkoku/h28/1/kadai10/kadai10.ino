#define USE_TIMER3
#include "mono_con.h"

const int tm = 100;
const int bz[5] = { 1,0,1,0,0 };
int idx = 0;

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

int n = 0, m = 0;
int move = 0;

bool mode = false;

word tc = 0, sp = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
	sp++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;
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

	if (!mode && tsw) {
		if (tc >= tm) {
			tc = 0;
			// move = 5;
			n++;
			if (n % 30 == 0) move = 5;
			if (n >= 60) {
				n = 0;
				m++;
			}
		}
		swps = false;
	} else {
		if (swps) {
			swps = false;
			mode = !mode;
		}

		if (!mode) {
			tc = tm;
			idx = 0;
			noTone(BZ_PIN);
		}
	}

	if (mode) {
		if (tc >= tm) {
			tc = 0;
			if (n > 0) {
				n--;
				if (n <= 0 && m > 0) {
					n = 59;
					m--;
					move = -5;
				}
				if (n != 0 && n % 30 == 0) move = -5;
			}
			if (bz[idx]) tone(BZ_PIN, 2000);
			else noTone(BZ_PIN);
			if (++idx > 4) idx = 0;
		}
	}

	if (sp >= 7) {
		sp = 0;
		if (move > 0) {
			move--;
			lm.spm(CW);
		} else if (move < 0) {
			move++;
			lm.spm(CCW);
		}
	}

	lm.flush();
	disp(num[(n % 100) / 10], num[(n % 100) % 10]);
}
