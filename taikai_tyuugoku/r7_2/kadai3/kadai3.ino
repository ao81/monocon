// 33:20

#define USE_TIMER3_ISR
#include "mono_con.h"

#define lo 0 + 200
#define hi 1023 - 200
#define MID(a) (a > lo && a < hi)

const int angles[8] = { 0, 15, 30, 45, 60, 75, 90, 105 };
const int tones[8] = { 523, 587, 659, 698, 784, 880, 988, 1047 };

int curdir = -1, predir = -1;
int curangle = 0;
int move = 0;

int phase = 0;

int xpos, ypos;
word in, spm, bz;

int getDir(int x, int y) {
	if (MID(x) && y < lo) return 0;
	else if (x < lo && y < lo) return 1;
	else if (x < lo && MID(y)) return 2;
	else if (x < lo && y > hi) return 3;
	else if (MID(x) && y > hi) return 4;
	else if (x > hi && y > hi) return 5;
	else if (x > hi && MID(y)) return 6;
	else if (x > hi && y < lo) return 7;
	else					   return -1;
}

int toAngle(int from, int to) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		xpos = analogRead(A1);
		ypos = analogRead(A2);
	}

	spm++;

	if (bz > 0) bz--;
	else noTone(BZ_PIN);
}

void setup() {
	config_init();
	serial_init();

	noTone(BZ_PIN);
}

void loop() {
	curdir = getDir(xpos, ypos);
	if (curdir != predir) {
		predir = curdir;
		if (curdir != -1) {
			bz = 100;
			tone(BZ_PIN, tones[curdir]);
		}
	}

	if (curdir != -1 && curangle != angles[curdir]) {
		if (spm > 20) {
			spm = 0;
			move = toAngle(curangle, angles[curdir]);
			if (move > 0) {
				lm.color.SM = stepm_init(phase);
				if (++phase > 3) phase = 0;
				curangle++;
				if (curangle >= 120) curangle -= 120;
			} else if (move < 0) {
				lm.color.SM = stepm_init(phase);
				if (--phase < 0) phase = 3;
				curangle--;
				if (curangle < 0) curangle += 120;
			}

			led_stepmotor(lm.b8);
		}
	}
}