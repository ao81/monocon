// 17:55

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw;
int pretsw, presw;
bool swps = false, ison = false;

int cnt = 0;

word in, tc;

void dispf(bool isOn, bool isLeft = true) {
	static bool pre_on = nullptr, pre_left = nullptr;
	if (pre_on != isOn || pre_left != isLeft) {
		pre_on = isOn, pre_left = isLeft;
		if (isOn) {
			if (isLeft) disp(0xff, 0x00);
			else disp(0x00, 0xff);
		} else disp(0x00, 0x00);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (tsw == HIGH && sw != presw) swps = true;

		presw = sw;
	}

	if (tsw == HIGH) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw || swps) {
			if (swps) swps = false;
			pretsw = tsw;
			dispf(false);
			tc = cnt = 0;
			ison = false;
		}

		if (cnt <= 3) {
			if (tc > 500) {
				tc = 0;
				ison = !ison;
				cnt++;
			}
		} else if (cnt <= 5) {
			if (tc > 2000) {
				tc = 0;
				ison = !ison;
				cnt++;
			}
		} else {
			cnt = 0;
			ison = false;
		}

		dispf(ison, (sw == LOW));

	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			dispf(false);
		}

	}
}