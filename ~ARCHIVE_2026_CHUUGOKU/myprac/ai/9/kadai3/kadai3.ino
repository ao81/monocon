#define USE_TIMER3
#include "mono_con.h"

const int angles[7] = { 0, 10, 20, 30, 40, 50, 60 };

const word V_MIN_DELAY = 8;
const word V_MAX_DELAY = 28;
const int  V_ACCEL = 15;

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;

int idx = 0;
int angle = 0;
int move = 0;
bool mv = false;

int total = 0;
int done = 0;
int dir = 0;

word tc;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

int getmove(int from, int to) {
	return to - from;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin3);
		sw = digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin3);
	sw = presw = digitalRead(pin4);
	ph = digitalRead(pin5);
}

void loop() {
	int i = getdir(x, y);
	static int prei = -1;

	if (prei != i) {
		if (prei == -1) {
			if (i == 1) {
				if (idx < 6) idx++;
			} else if (i == 3) {
				if (idx > 0) idx--;
			}
		}
		prei = i;
	}

	disp(0x00, num[idx]);

	if (tsw == HIGH) {
		mv = false;
		swps = false;
	} else if (swps) {
		swps = false;
		int d = getmove(angle, angles[idx]);
		dir = (d > 0) ? 1 : (d < 0 ? -1 : 0);
		total = abs(d);
		done = 0;
		mv = (total > 0);
		tc = 0;
	}

	if (mv) {
		int remaining = total - done;

		if (remaining <= 0) {
			mv = false;
			tone(BZ_PIN, 2000, 50);
		} else {
			int ramp = (done < remaining) ? done : remaining;
			if (ramp > V_ACCEL) ramp = V_ACCEL;

			word stepDelay = V_MIN_DELAY
				+ (word)((long)(V_MAX_DELAY - V_MIN_DELAY) * (V_ACCEL - ramp) / V_ACCEL);

			if (tc >= stepDelay) {
				tc = 0;
				angle += dir;
				done++;
				lm.spm(dir > 0 ? CW : CCW);
			}
		}
	}

	lm.flush();
}
