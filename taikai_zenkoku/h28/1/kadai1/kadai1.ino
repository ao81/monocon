#define USE_TIMER3
#include "mono_con.h"

const int angles[12] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110 };

int x, y, tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;
bool r = true;

int angle = 0, move = 0;

word cd = 1000, tc;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}

	if (sw == LOW) {
		if (cd > 0) cd--;
	} else cd = 1000;

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = !digitalRead(pin3);
	sw = presw = !digitalRead(pin4);
	ph = preph = !digitalRead(pin5);

	disp(0x00, num[0]);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = !digitalRead(pin3);
		sw = !digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	static int cnt = 0;

	if (phps) {
		phps = false;
		if (++cnt > 11) cnt = 0;
		if (cnt < 10) disp(0x00, num[cnt]);
		else disp(num[cnt / 10], num[cnt % 10]);
	}

	if (swps) {
		swps = false;
		if (cd != 0) { // 1秒未満
			int diff = (angles[cnt] - angle + 120) % 120;

			if (tsw == HIGH) { // cw
				move = diff;
			} else { // ccw
				move = diff - 120;
				if (diff == 0) move = 0;
			}

		} else {
			int diff = (angles[0] - angle + 120) % 120;

			if (tsw == HIGH) { // ccw
				move = diff - 120;
				if (diff == 0) move = 0;
			} else { // cw
				move = diff;
			}

			cnt = 0;
			disp(0x00, num[cnt]);
		}
	}

	if (tc >= 8) {
		tc = 0;
		if (move > 0) {
			move--;
			angle++;
			lm.spm(CW);
		} else if (move < 0) {
			move++;
			angle--;
			lm.spm(CCW);
		}
		angle = (angle + 120) % 120;
	}

	lm.flush();
}
