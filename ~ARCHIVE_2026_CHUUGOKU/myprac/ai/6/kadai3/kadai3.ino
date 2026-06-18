#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw;
int presw;
bool swps = false;

int angle = 0, phase = 0;
int move = 0;

word tc = 0;

int getidx(int xx, int yy) {
	int dx = xx - 512, dy = yy - 512;

	if ((long)dx * dx + (long)dy * dy < (long)200 * 200) return -1;

	double agl = atan2((double)dx, (double)dy);

	if (agl < 0) agl += 2 * PI;

	int idx = (int)((agl + PI / 16) / (2 * PI / 16));

	return idx % 16;
}

int getdir(int from, int to) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(A2);
		y = analogRead(A1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);
	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);

	delay(200);
}

void loop() {
	static int prei = -1;
	int i = getidx(x, y);

	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			tone(BZ_PIN, 1000, 100);
			move = 0;
			angle = 0;
		}

		if (i != prei) {
			prei = i;
			if (i != -1) {
				move = getdir(angle, i * 7.5f);
			}
		}

		if (tc >= 6) {
			tc = 0;
			if (move > 0) {
				if (--phase < 0) phase = 3;
				lm.spm(phase);
				move--;
				angle++;
			} else if (move < 0) {
				if (++phase > 3) phase = 0;
				lm.spm(phase);
				move++;
				angle--;
			}

			angle = (angle + 120) % 120;
		}
	} else {
		lm.spm(4);
		if (swps) swps = false;
	}

	lm.flush();
}
