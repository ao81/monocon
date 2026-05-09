// 4:52

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledpara[3] = { B001, B100, B010 };
int ledidx = 0;

int tsw;
int cnt = 0;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
	}

	tc++;
}

void onedisp(int n) {
	static int pren = -1;
	if (n != pren) {
		pren = n;
		disp(num[n / 10], num[n % 10]);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);

	disp(num[0], num[0]);
}

void loop() {
	if (tsw == HIGH) {
		if (tc >= 100) {
			tc = 0;
			if (++cnt > 99) cnt = 0;
			if (cnt % 10 == 0) {
				if (++ledidx > 2) ledidx = 0;
			}
		}
	}

	lm.color.GBR = ledpara[ledidx];
	led_stepmotor(lm.b8);
	onedisp(cnt);
}