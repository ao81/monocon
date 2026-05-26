#define USE_TIMER3
#include "mono_con.h"

int x, y;
word tc;
int angle = 0;

int gettarget(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if (abs(dx) < 200 && abs(dy) < 200) return -1;
	int t = (int)(atan2((float)dx, (float)dy) * 60 / PI);
	if (t < 0) t += 120;
	return t;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		x = analogRead(pin2);
		y = analogRead(pin1);
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (tc >= 7) {
		tc = 0;
		int t = gettarget(x, y);
		if (t >= 0) {
			int move = t - angle;
			if (move > 60) move -= 120;
			if (move < -60) move += 120;
			if (move > 0) {
				angle++;
				if (angle >= 120) angle = 0;
				lm.spm(CW).flush();
			}
			if (move < 0) {
				angle--;
				if (angle < 0) angle = 119;
				lm.spm(CCW).flush();
			}
		}
	}
}
