// 途中まで

#define USE_TIMER3
#include "mono_con.h"

const int tempo[3] = { 1000, 500, 300 };
int idx = 0;

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool input = true;

bool move = false;
word tc, re;

int getdir(int yy) {
	return constrain(map(yy, 0, 1023, 0, 3), 0, 2);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
	}
	tc++;

	if (re > 0) {
		re--;
		if (re == 0) {
			noTone(BZ_PIN);
			lm.led(B000);
		}
	}
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

	int i = getdir(y);
	static int prei = 1;

	if (i != prei) {
		if (prei == 1) {
			if (i == 0) {
				if (idx > 0) idx--;
			} else if (i == 2) {
				if (idx < 2) idx++;
			}
		}
		prei = i;
	}

	if (swps) {
		swps = false;
		move = !move;
	}

	static int cnt = 0;

	if (move) {
		if (tc >= tempo[idx]) {
			tc = 0;
			if (cnt == 0) {
				tone(BZ_PIN, 2000);
				lm.led(B001);
			} else {
				tone(BZ_PIN, 800);
				lm.led(B010);
			}
			re = 50;
			disp(0x00, num[cnt + 1]);
			if (++cnt > 3) cnt = 0;
		}
	} else {
		disp(0x00, 0x00);
		cnt = 0;
		tc = 1000;
	}

	lm.flush();
}
