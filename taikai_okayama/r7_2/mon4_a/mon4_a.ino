#define USE_TIMER3_ISR
#include "mono_con.h"

const int bz_pitch[2] = { 500, 1000 };
int idx = 0;

int count = 0;

word tc;

ISR(TIMER3_COMPA_vect) {
	tc++;
}

void setup() {
	config_init();
	serial_init();

	disp(num[0], num[0]);
}

void loop() {
	if (tc > 1000) {
		tc = 0;
		if (++idx > 1) idx = 0;
		if (++count > 99) count = 0;
		disp(num[count / 10], num[count % 10]);
	}
	tone(BZ_PIN, bz_pitch[idx]);
}