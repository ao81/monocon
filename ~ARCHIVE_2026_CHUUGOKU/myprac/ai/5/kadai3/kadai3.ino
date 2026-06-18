#define USE_TIMER3
#include "mono_con.h"
					//   6   5   4   7       3    8  1   2
const int angles[9] = { 75, 60, 45, 90, -1, 30, 105, 0, 15 };
const int speeds[9] = { -80, -1, 80, -160, 0, 80, -160, 160, 160 };
const int leds[9]	= { B001, B101, B101, B101, B000,  B100, B010, B011, B011 };
int angle = 0;
int sv_idx = 4;
int move = 0;
int phase = 0;
int x, y;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A2);
		y = analogRead(A1);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	y = analogRead(A1);
}

int getidx(int xx, int yy) {
	int xidx = constrain(map(xx, 0, 1023, 0, 3), 0, 2);
	int yidx = constrain(map(yy, 0, 1023, 0, 3), 0, 2);
	return xidx + yidx * 3;
}

int getdir(int from, int to) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
}

void cw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void loop() {
	int idx = getidx(x, y);
	static int preidx = -1;
	disp(num[idx], 0x00);

	if (idx != preidx) {
		preidx = idx;
		if (idx != 4) {
			sv_idx = idx;
			move = getdir(angle, angles[idx]);
			if (speeds[idx] > 0) cw(speeds[idx]);
			else if (speeds[idx] < 0) ccw(abs(speeds[idx]));
			else stop();
		};
	}

	if (tc >= 8) {
		tc = 0;
		if (move > 0) {
			if (--phase < 0) phase = 3;
			lm.spm(phase);
			move--;
			angle++;
		} else if (move < 0) {
			if (++phase > 3) phase = 0;
			lm.spm(phase);
			move++;
			angle--;
		}
	}

	lm.led(leds[sv_idx]);
	lm.flush();
}
