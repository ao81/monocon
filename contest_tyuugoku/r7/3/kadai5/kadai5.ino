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
int head = 0;

int idx = 4, preidx = -1;
int x, y;
int sw, presw;
bool swps = false;
int ph, preph;
bool phps = false;
word tc;

int blink = 0;
bool tgl = true;

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
		ph = digitalRead(pin_3);
		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;
		presw = sw;
		preph = ph;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);
	sw = presw = digitalRead(pin_4);
	ph = preph = digitalRead(pin_3);

	Serial.begin(57600);
}

void loop() {
	idx = getidx(x, y);
	if (idx != preidx) {
		preidx = idx;

		if (idx != 4) {
			if (head < 5) {
				tone(BZ_PIN, 1000, 50);
				saveidx[head++] = idx;
				Serial.println(lm);
			}
		} else noTone(BZ_PIN);
	}

	if (swps) {
		swps = false;

		head = 0;
		bool ok = false;
		for (int i = 0; i < 3; i++) {
			if (comp(saveidx, ptn[i])) {
				ok = true;
				okidx = i;
				break;
			}
		}
		if (ok) {
			cnt -= minas[okidx];
			blink = 0;
			tone(BZ_PIN, 1000, 1000);
		} else {
			blink = 3;
			tc = 200;
		}

		resetsv();
	}

	if (tc >= 200) {
		tc = 0;

		if (blink > 0) {
			tone(BZ_PIN, 1000, 100);
			blink--;
		}
	}

	if (phps) {
		phps = false;
		noTone(BZ_PIN);
		cnt = 10;
		blink = 0;
	}

	if (cnt >= 10) lm.led(B010);
	else if (cnt >= 4) lm.led(B101);
	else if (cnt >= 1) lm.led(B001);
	else lm.led(B000);

	lm.flush();
	if (cnt < 0) cnt = 0;
	disp(num[cnt / 10], num[cnt % 10]);
}
