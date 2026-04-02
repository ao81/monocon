#define USE_TIMER3_ISR
#include "mono_con.h"

volatile int cdMs = 0;
volatile int sw = HIGH;
int prevSw = HIGH;

ISR(TIMER3_COMPA_vect) {
	if (cdMs > 0) cdMs--;

	static int cnt = 0;
	if (++cnt >= 5) {
		cnt = 0;
		sw = digitalRead(_USER_CON_3PIN);
	}
}

void startCd(int sec) {
	cdMs = sec * 1000;
}

void updateDisp() {
	static int prev = -1;
	int sec = (cdMs + 999) / 1000;
	if (sec == prev) return;
	prev = sec;
	disp(num[sec], num[10]);
}

void setup() {
	config_init();
	serial_init();
	lm.color.GBR = B000;
	led_stepmotor(lm.b8);
	disp(num[0], num[10]);
}

void loop() {
	static int prevSw = HIGH;

	if (sw == LOW && prevSw == HIGH) {
		startCd(5);
		lm.color.GBR = B110;
	}

	if (cdMs == 0) {
		lm.color.GBR = B000;
	}

	led_stepmotor(lm.b8);
	updateDisp();

	prevSw = sw;
}
