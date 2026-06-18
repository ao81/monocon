#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw;

int cnt = 0;

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
		tsw = digitalRead(pin5);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin5);
}

void loop() {
	static int prei = -2;
	int i = getdir(x, y);
	if (i != prei) {
		if (prei == -1) {
			if (tsw == HIGH) {
				if (i == 0) cnt += 10;
				else if (i == 2) cnt += 5;
				else if (i == 4) cnt += 1;
			} else {
				if (i == 0) cnt -= 10;
				else if (i == 2) cnt -= 5;
				else if (i == 4) cnt -= 1;
			}
			cnt = constrain(cnt, 0, 99);
		}
		prei = i;
	}

	if (tsw) lm.led(B001);
	else lm.led(B010);
	lm.flush();

	disp(num[cnt / 10], num[cnt % 10]);
}
