// 7:51

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledPara[3] = { B001, B100, B010 };
int ledidx = 0;

int tsw;

int count = 0;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
	}

	if (tsw == HIGH) tc++;
	else tc = 100;
}

void setup() {
	config_init();
	serial_init();
	tsw = digitalRead(_USER_CON_5PIN);
	disp(num[0], num[0]);
	lm.color.GBR = B001;
	led_stepmotor(lm.b8);
}

void loop() {
	if (tsw == HIGH) {
		if (tc > 100) {
			tc = 0;
			if (++count > 99) count = 0;
			disp(num[count / 10], num[count % 10]);
			if (count % 10 == 0) {
				if (++ledidx > 2) ledidx = 0;
				lm.color.GBR = ledPara[ledidx];
			}
		}
		led_stepmotor(lm.b8);
	}
}