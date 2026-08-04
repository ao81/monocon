#define USE_TIMER3
#include "mono_con.h"

const int ptn[3][5] = {
	{ 4, 3, 2, -1, -1 },
	{ 2, 4, 3, -1, -1 },
	{ 2, 3, 4, 5, 6 },
};

int x, y;
int sw, ph;
int presw, preph;
bool swps = false, phps = false;

int buf[5] = { -1, -1, -1, -1, -1 };
int idx = 0;

int dir = -1;
int cnt = 10;

int bzcnt = 0;
word tc;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
}

void resetbuf() {
	for (int i = 0; i < 5; i++) buf[i] = -1;
}

int cmp(int buf[5]) {
	for (int i = 0; i < 3; i++) {
		bool ok = true;
		for (int j = 0; j < 5; j++) {
			if (ptn[i][j] != buf[j]) {
				ok = false;
				break;
			}
		}
		if (ok) return i;
	}
	return -1;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

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

	x = analogRead(pin2);
	y = analogRead(pin1);
	sw = presw = digitalRead(pin4);
	ph = digitalRead(pin3);

	disp(num[1], num[0]);
	lm.led(B010).flush();

	Serial.begin(9600);
}

void loop() {
	static int predir = -2;
	dir = getdir(x, y);
	if (dir != predir) {
		predir = dir;
		if (dir != -1 && idx < 5) {
			tone(BZ_PIN, 1500, 50);
			buf[idx++] = dir;
			Serial.println(dir);
		}
	}

	if (swps) {
		swps = false;

		int res = cmp(buf);
		if (res == -1) {
			bzcnt = 3;
			tc = 200;
		} else {
			tone(BZ_PIN, 2000, 1000);
			if (res == 0) cnt -= 2;
			else if (res == 1) cnt -= 3;
			else cnt -= 5;
			if (cnt < 0) cnt = 0;
		}

		idx = 0;
		resetbuf();
	}

	if (bzcnt > 0) {
		if (tc >= 200) {
			tc = 0;
			tone(BZ_PIN, 1600, 100);
			bzcnt--;
		}
	}

	if (phps) {
		phps = false;
		noTone(BZ_PIN);
		bzcnt = 0;
		resetbuf();
		idx = 0;
		cnt = 10;
	}

	if (cnt == 10) lm.led(B010);
	else if (cnt >= 4) lm.led(B101);
	else if (cnt >= 1) lm.led(B001);
	else lm.led(B000);
	lm.flush();

	disp(num[cnt / 10], num[cnt % 10]);
}
