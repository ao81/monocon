#define USE_TIMER3
#include "mono_con.h"

int x, y;

int getdir(int x_, int y_) {
	int dx = x_ - 511, dy = y_ - 511;
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
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
}

void loop() {
	int i = getdir(x, y);
	if (i == -1) disp(0x40, num[1]);
	else disp(0x00, num[i]);
}
