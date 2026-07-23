#define USE_TIMER3
#include "mono_con.h"

const int color[3] = { B100, B101, B001 };
const int cycle[3] = { 5000, 2000, 5000 };

int x, y;
int tsw, sw, ph;
int pretsw, presw;
bool swps = false;

int idx = 0;
int seg = 0;

bool force = false;
bool tgl = true;

bool night = false;

word tc = 0, cd = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin3);
		sw = digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	if (force || night) tc++;
	else {
		tc = 500;
		tgl = true;
	}
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = pretsw = digitalRead(pin3);
	sw = presw = digitalRead(pin4);
	ph = digitalRead(pin5);
}

void loop() {
	if (swps) {
		swps = false;
		force = !force;
		if (!force) {
			idx = 2;
			cd = cycle[idx];
		}
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		night = (tsw == HIGH);
		if (night) {
			tc = 1000;
			tgl = true;
		} else {
			idx = 2;
			cd = cycle[idx];
		}
	}

	if (!force) {
		if (!night) {
			if (cd <= 0) {
				if (++idx > 2) idx = 0;
				cd = cycle[idx];
				if (idx == 0) tone(BZ_PIN, 2000, 30);
			}

			seg = (cd + 999) / 1000;
			disp(0x00, num[seg]);

			lm.led(color[idx]).flush();
		} else {
			if (tc >= 1000) {
				tc = 0;
				if (tgl) lm.led(B101);
				else lm.led(B000);
				lm.flush();
				tgl = !tgl;
			}

			disp(0x00, 0x00);
		}

	} else {
		if (tc >= 500) {
			tc = 0;
			if (tgl) lm.led(B001);
			else lm.led(B000);
			lm.flush();
			tgl = !tgl;
		}
		disp(0x00, 0x00);
	}
}
