// 24:51

#define USE_TIMER3_ISR
#include "mono_con.h"

#define lo 0 + 200
#define hi 1023 - 200
#define MID(a) (a > lo && a < hi)

const int cmdPtn[3][5] = {
	{ 4, 3, 2, -1, -1 },
	{ 2, 4, 3, -1, -1 },
	{ 2, 3, 4, 5, 6 }
};
const int angles[8] = { 0, 15, 30, 45, 60, 75, 90, 105 };
int input[5] = { -1, -1, -1, -1, -1 };
int indir = 0, cmdIdx = 0;

int curdir = -1, predir = -1;
int curangle = 0;

int count = 10;

int sw, presw;
bool swps = false;

int xpos, ypos;
word in, spm, bz;
word bz_pause = 0, bz_rep = 0, bz_hz = 0;

void onedisp(int n) {
	static int prev = -1;
	if (prev != n) {
		disp(num[n / 10], num[n % 10]);
		prev = n;
	}
}

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

int isCommand(int arr[5]) {
	for (int i = 0; i < 3; i++) {
		bool isMatch = true;
		for (int j = 0; j < 5; j++) {
			if (cmdPtn[i][j] != arr[j]) {
				isMatch = false;
				break;
			}
		}
		if (isMatch) {
			return i;
		}
	}
	return -1;
}

int toAngle(int from, int to) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
}

void initInput() {
	for (int i = 0; i < 5; i++) {
		input[i] = -1;
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		xpos = analogRead(A1);
		ypos = analogRead(A2);
		sw = digitalRead(_USER_CON_5PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	spm++;

	if (bz > 0) {
		bz--;
		if (bz == 0) {
			noTone(BZ_PIN);
			if (bz_rep > 0) bz_pause = 100;
		}
	} else if (bz_pause > 0) {
		bz_pause--;
		if (bz_pause == 0 && bz_rep > 0) {
			tone(BZ_PIN, bz_hz);
			bz = 100;
			bz_rep--;
		}
	} else noTone(BZ_PIN);
}

void setup() {
	config_init();
	serial_init();

	xpos = analogRead(A1);
	ypos = analogRead(A2);
	sw = presw = digitalRead(_USER_CON_5PIN);

	disp(num[1], num[0]);
	lm.color.GBR = B010;
	led_stepmotor(lm.b8);
	noTone(BZ_PIN);
}

void loop() {
	curdir = getDir(xpos, ypos);
	if (curdir != predir) {
		predir = curdir;
		if (curdir != -1) {
			bz = 70;
			tone(BZ_PIN, 1000);
			if (indir < 5) input[indir++] = curdir;
		}
	}

	if (swps) {
		swps = false;
		bz_rep = 0;
		indir = 0;

		cmdIdx = isCommand(input);
		if (cmdIdx == 0) { count -= 2; bz = 1000; tone(BZ_PIN, 1000); }
		else if (cmdIdx == 1) { count -= 3; bz = 1000; tone(BZ_PIN, 1000); }
		else if (cmdIdx == 2) { count -= 5; bz = 1000; tone(BZ_PIN, 1000); }
		else {
			bz_hz = 1000;
			tone(BZ_PIN, bz_hz);
			bz = 100;
			bz_rep = 2;
		}
		if (count < 0) count = 0;

		if (count >= 10) lm.color.GBR = B010;
		else if (count >= 4) lm.color.GBR = B101;
		else if (count >= 1) lm.color.GBR = B001;
		else lm.color.GBR = B000;

		led_stepmotor(lm.b8);

		onedisp(count);
		initInput();
	}
}