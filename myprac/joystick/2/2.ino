#define USE_TIMER3
#include "mono_con.h"

int x, y, sw;
int angle = 0;
word tc;

int getangle(int x_, int y_) {
	static const int dz = 200;
	int dx = x_ - 511;
	int dy = y_ - 511;

	if (abs(dx) < dz && abs(dy) < dz) return 0;

	float rad = atan2((float)dx, (float)dy);
	int a = (int)(rad * 60.0 / PI);

	if (a < 0) a += 120;
	if (a >= 120) a -= 120;

	return a;
}

int getStep(int current, int x_, int y_) {
	int target = getangle(x_, y_);
	int diff = target - current;

	// -60 〜 +59 に正規化
	if (diff > 60)  diff -= 120;
	if (diff < -60) diff += 120;

	if (diff > 0) return 1;
	if (diff < 0) return -1;
	return 0;
}

bool stepAngle(int& angle, int x_, int y_) {
	int target = getangle(x_, y_);
	int diff = target - angle;

	if (diff > 60)  diff -= 120;
	if (diff < -60) diff += 120;

	if (diff > 0) {
		angle = (angle + 1) % 120;
		lm.spm(CW);
		return true;
	} else if (diff < 0) {
		angle = (angle + 119) % 120;
		lm.spm(CCW);
		return true;
	}
	return false;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		sw = digitalRead(pin4);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	sw = digitalRead(pin4);
}

void loop() {
	if (tc >= 7) {
		tc = 0;
		stepAngle(angle, x, y);
		lm.flush();
	}

	if (!sw) lm.spm(STOP).flush();
}
