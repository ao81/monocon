#define USE_TIMER3
#include "mono_con.h"

const int bz[8] = { 523, 587, 659, 698, 784, 880, 988, 1047 };
int x, y;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if (abs(dx) < 200 && abs(dy) < 200) return -1;
	int t = (int)((atan2((float)dx, (float)dy) + PI / 8) * 4 / PI);
	if (t < 0) t += 8;
	if (t > 7) t -= 8;
	return t;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
}

void loop() {
	static int prei = -2;
	int i = getdir(x, y);
	if (i != prei) {
		prei = i;
		if (i == -1) {
			noTone(BZ_PIN);
		} else {
			tone(BZ_PIN, bz[i]);
		}
	}
}
