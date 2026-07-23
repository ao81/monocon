// 途中まで

#define USE_TIMER3
#include "mono_con.h"

const int ptn[3][5] = {
	{ 1, 2, 5, -1, -1 },
	{ 5, 1, 2, -1, -1 },
	{ 5, 2, 1, 0, 3 }
};
const int minas[3] = { 2, 3, 6 };

const int angles[9] = { 75, 60, 45, 90, -1, 30, 105, 0, 15 };
int saveidx[5] = { -1, -1, -1, -1, -1 };

int idx = 4, preidx = -1;
int x, y;
int sw, presw;
bool swps = false;
word tc;

int cnt = 10;
int okidx = 0;

int getidx(int xx, int yy) {
	int xidx = constrain(map(xx, 0, 1023, 0, 3), 0, 2);
	int yidx = constrain(map(yy, 0, 1023, 0, 3), 0, 2);
	return xidx + yidx * 3;
}

void resetsv() {
	for (int i = 0; i < 5; i++) saveidx[i] = -1;
}

bool comp(const int a[5], const int b[5]) {
	for (int i = 0; i < 5; i++) {
		if (a[i] != b[i]) return false;
	}
	return true;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		x = analogRead(A2);
		y = analogRead(A1);
		sw = digitalRead(pin_4);
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
	sw = presw = digitalRead(pin_4);
}

void loop() {
	idx = getidx(x, y);
	if (idx != preidx) {
		preidx = idx;

		if (idx != 4) {
			tone(BZ_PIN, 1000, 50);
		} else noTone(BZ_PIN);

		if (swps) {
			swps = false;
			bool ok = true;
			for (int i = 0; i < 3; i++) {
				if (!comp(saveidx, ptn[i])) {
					ok = false;
					okidx = i;
				}
			}
			if (ok) {
				cnt -= minas[okidx];
				tone(BZ_PIN, 2000, 1000);
			}
			resetsv();
		}
	}

	lm.update();
	if (cnt < 0) cnt = 0;
	disp(num[cnt / 10], num[cnt % 10]);
}
