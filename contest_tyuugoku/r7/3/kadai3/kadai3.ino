#define USE_TIMER3
#include "mono_con.h"

const int angles[9] = { 75, 60, 45, 90, -1, 30, 105, 0, 15 };
const int pitches[9] = { 880, 784, 698, 988, -1, 659, 1047, 523, 587 };

int idx = 4, preidx = -1;
int angle = 0, move = 0;
int phase = 0;
int x, y;
word tc;

int getidx(int xx, int yy) {
	int xidx = constrain(map(xx, 0, 1023, 0, 3), 0, 2);
	int yidx = constrain(map(yy, 0, 1023, 0, 3), 0, 2);
	return xidx + yidx * 3;
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
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);
}

void loop() {
	idx = getidx(x, y);
	if (idx != preidx) {
		preidx = idx;
		if (idx != 4) {
			tone(BZ_PIN, pitches[idx]);
			move = getdir(angle, angles[idx]);
		} else noTone(BZ_PIN);
	}

	if (tc >= 7) {
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

	lm.update();
}
