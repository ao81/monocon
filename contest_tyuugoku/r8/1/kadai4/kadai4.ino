#define USE_TIMER3
#include "mono_con.h"

const int led[8] = { B001, B011, B100, B111, B010, B110, B100, B101 };
int data[500] = { 0 };

int idx = 0;
int hz = 0;

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
int swps = false, phps = false;
bool r = true;
word tc = 0, cd = 0;

typedef enum {
	no,
	wait,
	now,
	end,
	play,
} status;
status st;

int getdir(int xx, int yy) {
	long dx = xx - 511, dy = yy - 511;
	if (dx * dx + dy * dy < 20000) return -1;
	return (int)((atan2(dx, dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
}

int gethz(int xx, int yy) {
	int hz1 = map(yy, 0, 1023, 100, 1500);
	int hz2 = map(xx, 0, 1023, -50, 50);
	return hz1 + hz2;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
	if (cd > 0) cd--;
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

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == LOW && preph == HIGH) phps = true;

		presw = sw;
		preph = ph;
	}

	if (sw == LOW) {
		noTone(BZ_PIN);
		hz = 0;

	} else {
		int i = getdir(x, y);
		hz = gethz(x, y);
		if (i != -1) {
			if (!tsw || st == now) {
				tone(BZ_PIN, hz);
				lm.led(led[i]);
			} else {
				lm.led(B000);
			}
		} else {
			if (!tsw || st == now) {
				tone(BZ_PIN, 800);
				lm.led(B000);
			} else {
				lm.led(B000);
			}
		}
	}

	if (tsw == HIGH) { // 録音
		switch (st) {
		case no:
			if (swps) {
				swps = false;
				st = wait;
				cd = 1000;
			}

			noTone(BZ_PIN);

			break;
		case wait:
			if (cd <= 0) {
				tc = 0;
				st = now;
				idx = 0;
			}

			break;
		case now:
			if (tc >= 10 && idx < 500) {
				tc = 0;
				if (sw == LOW) data[idx] = -1;
				else data[idx] = hz;

				if (++idx > 499) {
					st = end;
					swps = false;
					phps = false;
				}
			}

			break;
		case end:
			if (swps) {
				swps = false;
				idx = 0;
				tc = 0;
				st = play;
			}

			if (phps) {
				phps = false;
				for (int i = 0; i < 500; i++) data[i] = -1;
				st = no;
			}

			noTone(BZ_PIN);

			break;
		case play:
			if (tc >= 10) {
				tc = 0;
				if (data[idx] == -1) noTone(BZ_PIN);
				else tone(BZ_PIN, data[idx]);

				if (++idx > 499) {
					st = end;
					noTone(BZ_PIN);
					phps = false;
				}
			}

			if (swps) {
				swps = false;
				noTone(BZ_PIN);
				st = end;
			}

			break;
		}

		disp(num[0], num[(int)st]);

	} else { // 演奏
		st = no;
		disp(num[0], num[0]);
		swps = phps = false;
	}

	lm.flush();
}
