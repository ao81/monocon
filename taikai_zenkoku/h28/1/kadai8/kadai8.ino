#define USE_TIMER3
#include "mono_con.h"

const int seg[8][2] = {
	{ 0x30, 0x00 },
	{ 0x21, 0x00 },
	{ 0x01, 0x01 },
	{ 0x00, 0x03 },
	{ 0x00, 0x06 },
	{ 0x00, 0x0c },
	{ 0x08, 0x08 },
	{ 0x18, 0x00 },
};
int idx = 0;

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
int phps = false;
bool r = true;

int move = 0;

word tc = 0, t = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
	t++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = !digitalRead(pin3);
	sw = presw = !digitalRead(pin4);
	ph = preph = digitalRead(pin5);

	disp(seg[0][0], seg[0][1]);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = !digitalRead(pin3);
		sw = !digitalRead(pin4);
		ph = digitalRead(pin5);

		if (ph == HIGH && preph == LOW) phps = true;

		preph = ph;
	}

	if (sw == LOW) {
		if (tc >= 1000) {
			tc = 0;
			if (tsw == LOW) {
				if (++idx > 7) idx = 0;
			} else {
				if (--idx < 0) idx = 7;
			}
		}

	} else {
		tc = 1000;
	}

	if (phps) {
		phps = false;
		if (idx >= 0 && idx <= 3) {
			move += 120;
		} else {
			move += -120;
		}
	}

	if (t >= 8) {
		t = 0;
		if (move > 0) {
			move--;
			lm.spm(CW).flush();
		} else if (move < 0) {
			move++;
			lm.spm(CCW).flush();
		}
	}

	disp(seg[idx][0], seg[idx][1]);
}
