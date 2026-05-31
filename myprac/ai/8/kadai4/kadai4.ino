// 途中まで

#define USE_TIMER3
#include "mono_con.h"

const int agl[4] = { 0, 20, 40, 60 };
const int dir[8] = { 3, 2, 1, -1, 0, -1, -1, -1 };
int x, y;

bool moving = false;
int angle = 0, move = 0;

word tc;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
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

	x = analogRead(pin2);
	y = analogRead(pin1);
}

/*
7 0 1
6 - 2
5 4 3
*/
void loop() {
	int i = getdir(x, y);
	static int prei = -1;
	if (i != prei) {
		if (prei == -1) {
			if (i == 4 || (i >= 0 && i <= 2)) {
				move = agl[dir[i]] - angle;
			}
		}
		prei = i;
	}

	if (tc >= 7) {
		tc = 0;
		if (move > 0) {
			move--;
			lm.spm(CCW);
			angle++;
		} else if (move < 0) {
			move++;
			lm.spm(CW);
			angle--;
		}
		lm.flush();
	}
}
