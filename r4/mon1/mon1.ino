#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledParam[3] = { B001, B100, B000 };

volatile int TSW;
int prevTSW;

bool blinking = false;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;
		TSW = digitalRead(_USER_CON_4PIN);
	}
	in++;
	tc++;
}

void setup() {
	config_init();
	serial_init();

	TSW = prevTSW = digitalRead(_USER_CON_4PIN);
}

void loop() {
	static int nowcnt = 0, cnt = 0, tgl = true;

	if (TSW != prevTSW) {
		prevTSW = TSW;
		cnt++;
		nowcnt = 0;
		tgl = true;
		blinking = true;
	}

	if (blinking && cnt > nowcnt) {
		if (tc > 500) {
			tc = 0;
			if (tgl) {
				lm.color.GBR = ledParam[cnt % 2];
				tgl = false;
			} else {
				lm.color.GBR = ledParam[2];
				nowcnt++;
				tgl = true;
			}
		}
	}

	led_stepmotor(lm.b8);
}