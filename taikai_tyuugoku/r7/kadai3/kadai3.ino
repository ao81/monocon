#define USE_TIMER3_ISR
#include "mono_con.h"

const int angleList[8] = { 0, 15, 30, 45, 60, 75, 90, 105 };
const int pitchList[8] = { 523, 587, 659, 698, 784, 880, 987, 1047 };

const int lowEdge = 0 + 300;
const int mid = 512;
const int highEdge = 1023 - 300;

int dir = -1, predir = -1;
int spm_angle = 0;
int move = 0;

int phase = 0;

// 0~1023
int x_pos, y_pos;

word in, spm;

int getDir(int x, int y) {
	if (x > lowEdge && x < highEdge && y < lowEdge) return 0;
	else if (x < lowEdge && y < lowEdge) return 1;
	else if (x < lowEdge && y > lowEdge && y < highEdge) return 2;
	else if (x < lowEdge && y > highEdge) return 3;
	else if (x > lowEdge && x < highEdge && y > highEdge) return 4;
	else if (x > highEdge && y > highEdge) return 5;
	else if (x > highEdge && y > lowEdge && y < highEdge) return 6;
	else if (x > highEdge && y < lowEdge) return 7;
	else return -1;
}

int toNextAngle(int cur, int target) {
	int diff = target - cur;
	diff %= 120;
	if (diff < 0) diff += 120;
	if (diff >= 60) diff -= 120;
	return diff;
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x_pos = analogRead(_USER_CON_1PIN);
		y_pos = analogRead(_USER_CON_2PIN);
	}
	spm++;
}

void setup() {
	config_init();
	serial_init();

	x_pos = analogRead(_USER_CON_1PIN);
	y_pos = analogRead(_USER_CON_2PIN);
}

void loop() {
	dir = getDir(x_pos, y_pos);

	if (dir != predir) {
		predir = dir;
		if (dir != -1) {
			tone(BZ_PIN, pitchList[dir]);
		} else {
			noTone(BZ_PIN);
		}
	}

	if (dir != -1 && spm_angle != angleList[dir]) {
		if (spm > 40) {
			spm = 0;
			move = toNextAngle(spm_angle, angleList[dir]);

			if (move > 0) {
				lm.color.SM = stepm_init(phase);
				if (++phase > 3) phase = 0;
				spm_angle++;
				if (spm_angle >= 120) spm_angle -= 120;

			} else if (move < 0) {
				lm.color.SM = stepm_init(phase);
				if (--phase < 0) phase = 3;
				spm_angle--;
				if (spm_angle < 0) spm_angle += 120;
			}

			led_stepmotor(lm.b8);
		}
	}
}