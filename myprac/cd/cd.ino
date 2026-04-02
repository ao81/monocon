#define USE_TIMER3_ISR
#include "mono_con.h"

int sw = HIGH, presw = HIGH;
int cd = -1, in = 0;

ISR(TIMER3_COMPA_vect) {
	if (++in > 5) {
		in = 0;
		sw = digitalRead(18);
	}
	if (cd > 0) cd--;
}

void startCd(int s) {
	cd = s * 1000;
}

void updateCd() {
	static int prevDisp = -1;
	int currDisp = (cd + 999) / 1000;
	if (currDisp != prevDisp) {
		disp(num[currDisp], num[10]);
		prevDisp = currDisp;
	}
}

void setup() {
	config_init();
	serial_init();
	lm.color.GBR = B000;
}

void loop() {
	if (sw == LOW && presw == HIGH) {
		startCd(5);
		lm.color.GBR = B110;
	} else if (cd == 0) {
		lm.color.GBR = B000;
	}

	led_stepmotor(lm.b8);

	updateCd();

	presw = sw;
}