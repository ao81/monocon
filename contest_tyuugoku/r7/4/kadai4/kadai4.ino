#define USE_TIMER3
#include "mono_con.h"

int sw, ph;
int presw, preph;
bool swps = false, phps = false;

bool move = false;
int cnt = 0;
bool t = false;
int c = 0;
bool visible = true;

word tc, bz;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
	bz++;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	disp(num[0], num[0]);
}

void loop() {
	if (phps) {
		phps = false;
		if (!move) {
			move = true;
			tc = 0;
			lm.led(B000);
			cnt = 0;
			t = false;
			visible = false;
		}
	}

	if (move) {
		if (tc >= 1000) {
			tc = 0;
			if (++cnt > 99) cnt = 0;
		}

		if (ph == LOW) {
			move = !move;

			if (cnt == 10) {
				lm.led(B010);
				tone(BZ_PIN, 2000, 2000);
			} else if (cnt == 9 || cnt == 11) {
				lm.led(B101);
				t = true;
				c = 3;
				bz = 200;
			} else {
				lm.led(B001);
				tone(BZ_PIN, 800, 1000);
			}
		}
	}

	if (t) {
		if (bz >= 200) {
			bz = 0;
			tone(BZ_PIN, 2000, 100);
			if (--c <= 0) t = false;
		}
	}

	if (swps) {
		swps = false;
		visible = true;
	}

	if (!move || visible) disp(num[cnt / 10], num[cnt % 10]);
	else if (cnt >= 3) disp(0x00, 0x00);
	else disp(num[cnt / 10], num[cnt % 10]);
}
