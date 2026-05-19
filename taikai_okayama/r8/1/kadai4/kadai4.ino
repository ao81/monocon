#define USE_TIMER3_ISR
#include "mono_con.h"

const int seg[2] = { 0x74, 0x5c };
const int angles[9] = { 105, 90, 75, 0, -1, 60, 15, 30, 45 };

int x, y;
int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;

bool move = false;

int phase = 0;
int angle = 0;
int preidx = 4;

bool tgl = true;
bool ok = false;

int to = 0;

word tc;

int getidx(int p) {
	return constrain(map(p, 0, 1023, 3, 0), 1, 3);
}

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
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
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == LOW && preph == HIGH) phps = true;

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
	tsw = digitalRead(_USER_CON_5PIN);
	pretsw = !tsw;
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);

	Serial.begin(9600);
}

void loop() {
	if (pretsw != tsw) {
		pretsw = tsw;
		if (tsw == LOW) {
			lm.color.GBR = B001;
			move = false;
			ok = false;
			swps = phps = false;
		} else lm.color.GBR = B100;
	}

	if (tsw == LOW) { // 原点復帰モード
		if (swps) {
			swps = false;
			if (!ok) {
				move = true;
				tgl = true;
			}
			phps = false;
		}

		if (move) {
			if (tc >= 200) {
				tc = 0;
				cw();
				if (tgl) lm.color.GBR = B101;
				else lm.color.GBR = B000;
				tgl = !tgl;
			}

			if (phps) {
				phps = false;
				move = false;
				ok = true;
				angle = 0;
				to = 0;
			}
		} else if (!ok) {
			lm.color.GBR = B001;
		} else {
			lm.color.GBR = B100;
		}

		disp(seg[0], seg[1]);

	} else { // 操作モード
		int xidx = getidx(1023 - x);
		int yidx = getidx(y);
		int idx = (xidx - 1) + (yidx - 1) * 3;

		if (idx != preidx) {
			preidx = idx;
			Serial.println(idx);
			if (angles[idx] == -1) to = 0;
			else to = getdir(angle, angles[idx]);
		}

		if (angle == 0) lm.color.GBR = B100;
		else if (to != 0) lm.color.GBR = B101;
		else lm.color.GBR = B001;

		if (tc >= 8) {
			tc = 0;
			if (to > 0) {
				cw();
				to--;
				angle++;
			} else if (to < 0) {
				ccw();
				to++;
				angle--;
			}
			angle = (angle + 120) % 120;
		}

		disp(num[yidx], num[4 - xidx]);
	}

	led_stepmotor(lm.b8);
}
