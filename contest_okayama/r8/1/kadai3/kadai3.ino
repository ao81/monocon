#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;

int phase = 0;
int cnt = 0;
bool tgl = 1;
bool move = false;
bool iscw = true;

int count = 0;

word tc, wait;

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
	wait++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	pretsw = !tsw;
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);

	disp(num[0], 0x00);
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		tgl = true;
		swps = phps = false;
		move = false;
		
	}

	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			if (!(move && iscw)) {
				move = !move;
				iscw = false;
				cnt = -12;
				wait = 500;
				tc = 40;
			}
		}

		if (phps) {
			phps = false;
			if (!(move && !iscw)) {
				move = !move;
				iscw = true;
				cnt = 12;
				wait = 500;
				tc = 40;
			}
		}


		if (move) {
			if (wait >= 500) {
				wait = 0;
				if (tgl) {
					if (iscw) cnt = 12;
					else cnt = -12;
					if (++count > 99) count = 0;
				}
				tgl = !tgl;
			}

			if (tc >= 40) {
				tc = 0;

				if (cnt > 0) {
					cw();
					cnt--;
				} else if (cnt < 0) {
					ccw();
					cnt++;
				}
			}
		} else {
			wait = 500;
			tc = 40;
		}

	} else {
		if (ph == LOW && swps) {
			swps = false;
			cnt = -4;
			if (++count > 99) count = 0;
		}

		if (sw == HIGH && phps) {
			phps = false;
			cnt = 4;
			if (++count > 99) count = 0;
		}

		if (tc >= 40) {
			tc = 0;

			if (cnt > 0) {
				cw();
				cnt--;
			} else if (cnt < 0) {
				ccw();
				cnt++;
			}
		}
	}

	if (count < 10) disp(0x00, num[count % 10]);
	else disp(num[count / 10], num[count % 10]);
	led_stepmotor(lm.b8);
}
