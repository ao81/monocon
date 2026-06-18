#define USE_TIMER3
#include "mono_con.h"

/*
6 7 8
3 4 5
0 1 2
*/

const int cmda[4] = { 7, 5, 1, 3 };
const int cmdb[4] = { 8, 2, 0, 6 };
const int cmdc[4] = { 7, 7, 1, 1 };
const int* cmds[3] = { cmda, cmdb, cmdc };

const int mv[3] = { 30, 60, 90 };

int x, y;
int sw, ph;
int presw, preph;
bool swps = false, phps = false;

int inidx[4] = { -1, -1, -1, -1 };
int cnt = 0;

int blink = 0;
int tgl = 0;

int idx = -1, preidx = -1;
bool success = false;

int move = 0;
int phase = 0;

word tc, sp;

int getidx(int xx, int yy) {
	int xi = constrain(map(xx, 0, 1023, 0, 3), 0, 2);
	int yi = constrain(map(yy, 0, 1023, 0, 3), 0, 2);
	return xi + yi * 3;
}

void resetbuf() {
	for (int i = 0; i < 4; i++) inidx[i] = -1;
}

int cmp(int inp[4]) {
	for (int i = 0; i < 3; i++) {
		bool ok = true;
		for (int j = 0; j < 4; j++) {
			if (inp[j] != cmds[i][j]) ok = false;
		}
		if (ok) return i;
	}
	return -1;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(A2);
		y = analogRead(A1);
		sw = digitalRead(pin_4);
		ph = digitalRead(pin_3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
	sp++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);
	sw = presw = digitalRead(pin_4);
	ph = digitalRead(pin_3);

	idx = preidx = getidx(x, y);
	resetbuf();

	disp(num[0], num[0]);
	lm.led(B001);
}

void loop() {
	idx = getidx(x, y);

	if (idx != preidx) {
		preidx = idx;
		if (!success && move == 0 && cnt < 4 && idx != 4) {
			tone(BZ_PIN, 1000, 80);
			inidx[cnt++] = idx;
		}
	}

	if (swps) {
		swps = false;

		if (move == 0 && !success) {
			int res = cmp(inidx);
			if (res == -1) {
				tone(BZ_PIN, 800, 1000);
				blink = 3;
				tc = 200;
				cnt = 0;
				tgl = 0;
				resetbuf();
				success = false;
			} else {
				lm.led(B100);
				tone(BZ_PIN, 2000, 1000);
				success = true;
				move = mv[res];
				cnt = 1;
			}
		}
	}

	if (tc >= 200) {
		tc = 0;

		if (blink > 0) {
			if (tgl) {
				lm.led(B001);
				blink--;
			} else {
				lm.led(B000);
			}
			tgl = !tgl;
		}
	}

	if (sp >= 30) {
		sp = 0;
		if (move > 0) {
			move--;
			if (--phase < 0) phase = 3;
			lm.spm(phase);
		}
	}

	if (phps) {
		phps = false;
		if (success) {
			success = false;
			cnt = 0;
			resetbuf();
			noTone(BZ_PIN);
			lm.led(B001);
		}
	}

	lm.flush();
	disp(0x00, num[cnt]);
}
