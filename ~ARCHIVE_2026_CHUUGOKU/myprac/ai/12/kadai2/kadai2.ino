#define USE_TIMER3
#include "mono_con.h"

const int led[3] = { B010, B100, B001 };
const int dm[3] = { 70, 100, 130 };
const int mxangle[3] = { 20, 13, 6 };

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool r = true;

int power = 0;
bool move = false;

int angle = 0, mv = 0;
int dir = 1;
int aidx = 0;

word tc;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI / PI / 4) / (PI / 2)) % 4;
}

int getmv(int from, int to) {
	return to - from;
}

void cw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();
	sw = presw = digitalRead(pin4);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	if (swps) {
		swps = false;
		move = !move;
	}

	if (!move) {
		lm.led(B000).flush();
		disp(0x00, 0x00);
		stop();

	} else {
		int i = getdir(x, y);
		static int prei = 1;

		if (i != prei) {
			if (i == 2) { // 下
				if (power > 0) power--;
			} else if (i == 0) { // 上
				if (power < 2) power++;
			} else if (i == 1) {
				if (++aidx > 2) aidx = 0;
			}
			prei = i;
		}

		if (tsw == HIGH) {
			if (tc >= 30) {
				tc = 0;
				if (dir == 1) {
					if (++angle >= mxangle[aidx]) dir = -dir;
					lm.spm(CW);
				} else {
					if (--angle <= 0) dir = -dir;
					lm.spm(CCW);
				}
			}
		}

		cw(dm[power]);
		lm.led(led[power]).flush();
		disp(0x00, num[power + 1]);
	}
}
