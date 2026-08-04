#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn[3][5] = {
	{ 4, 3, 2, -1, -1 },
	{ 2, 4, 3, -1, -1 },
	{ 2, 3, 4, 5, 6 },
};

const int lowEdge = 0 + 300;
const int mid = 512;
const int highEdge = 1023 - 300;

int cnt = 10;
int bz_rep = 0, bz_wait = 0;

int inIdx = 0;
int inputCmd[5] = { -1, -1, -1, -1, -1 };

int dir = -1, predir = -1;

// 0~1023
int x_pos, y_pos;
int sw, presw, ph, preph;
bool swps = false, phps = false;

word in, bz;

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

int isCommand(int arr[5]) {
	for (int i = 0; i < 3; i++) {
		bool ok = true;
		for (int j = 0; j < 5; j++) {
			if (arr[j] != ptn[i][j]) {
				ok = false;
				break;
			}
		}
		if (ok) return i;
	}
	return -1;
}

int pre_n;
void onedisp(int n) {
	if (pre_n != n) {
		disp(num[n / 10], num[n % 10]);
		pre_n = n;
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x_pos = analogRead(_USER_CON_1PIN);
		y_pos = analogRead(_USER_CON_2PIN);

		sw = digitalRead(_USER_CON_5PIN);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;

		ph = digitalRead(_USER_CON_3PIN);
		if (ph == HIGH && preph == LOW) phps = true;
		preph = ph;
	}

	if (bz > 0) bz--;
	else noTone(BZ_PIN);

	if (bz_rep > 0) {
		if (bz_wait > 0) {
			bz_wait--;
			if (bz_wait == 0) {
				tone(BZ_PIN, 1000);
				bz = 200;
				bz_rep--;
			}
		} else if (bz == 0) {
			noTone(BZ_PIN);
			bz_wait = 150;
		}
	}
}

void setup() {
	config_init();
	serial_init();

	x_pos = analogRead(_USER_CON_1PIN);
	y_pos = analogRead(_USER_CON_2PIN);
	sw = presw = digitalRead(_USER_CON_5PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);

	disp(num[1], num[0]);

	lm.color.GBR = B010;
	led_stepmotor(lm.b8);
}

void loop() {
	dir = getDir(x_pos, y_pos);

	if (dir != predir) {
		predir = dir;
		if (dir != -1) {
			tone(BZ_PIN, 1000);
			bz = 80;
			if (inIdx < 5) inputCmd[inIdx++] = dir;
		} else {
			noTone(BZ_PIN);
		}
	}

	if (phps) {
		phps = false;
		for (int i = 0; i < 5; i++) inputCmd[i] = -1;
		inIdx = 0;
		cnt = 10;
		noTone(BZ_PIN);
		lm.color.GBR = B010;
	}

	if (swps) {
		swps = false;

		switch (isCommand(inputCmd)) {
		case 0:
			cnt -= 2;
			tone(BZ_PIN, 1000);
			bz = 1000;
			break;
		case 1:
			cnt -= 3;
			tone(BZ_PIN, 1000);
			bz = 1000;
			break;
		case 2:
			cnt -= 5;
			tone(BZ_PIN, 1000);
			bz = 1000;
			break;
		default:
			bz_rep = 2;
			bz_wait = 0;
			bz = 200;
			tone(BZ_PIN, 1000);
			break;
		}

		if (cnt < 0) cnt = 0;

		if (cnt == 10) lm.color.GBR = B010;
		else if (cnt >= 4) lm.color.GBR = B101;
		else if (cnt >= 1) lm.color.GBR = B001;
		else lm.color.GBR = B000;

		for (int i = 0; i < 5; i++) inputCmd[i] = -1;
		inIdx = 0;
	}

	led_stepmotor(lm.b8);
	onedisp(cnt);
}