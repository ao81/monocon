// 途中まで

#define USE_TIMER3
#include "mono_con.h"

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
int swps = false, phps = false;
bool r = true;

word cd = 0, led = 0;

int getdir(int xx, int yy) {
	long dx = xx - 511, dy = yy - 511;
	if (dx * dx + dy * dy < 20000) return -1;
	return (int)((atan2(dx, dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}

	if (cd > 0) cd--;
	if (led > 0) {
		led--;
		if (led == 0) lm.led(B000);
	}
}

void setup() {
	config_init();
	serial_init();
	randomSeed(analogRead(0));
}

void loop() {
	if (r) {
		r = false;

		y = 1023 - analogRead(pin2);
		x = 1023 - analogRead(pin1);
		tsw = digitalRead(pin3);
		sw = digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	static bool game = false;
	static int n = -1;
	static int score = 0;

	if (!game) {
		if (swps) {
			swps = false;
			game = true;
			n = random(1, 9);
			cd = 2000;
		}

		disp(0x00, 0x00);

	} else {
		int i = getdir(x, y);

		auto ng = [](){
			lm.led(B001);
			tone(BZ_PIN, 800, 1000);
		};

		if (cd > 0) {
			if (i != -1) {
				if (i == n - 1) { // ok
					score++;
					tone(BZ_PIN, 2000, 1000);
					lm.led(B100);
					led = 100;

				} else ng();
			}
		} else ng();

		disp(num[n], num[score]);
	}
}
