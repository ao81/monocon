#define USE_TIMER3_ISR
#include "mono_con.h"

int tgl = 1;
word tc;

ISR (TIMER3_COMPA_vect) {
	tc++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (tc > 500) {
		tc = 0;

		if (tgl) disp(num[8], num[8]);
		else disp(num[10], num[10]);

		tgl ^= 1;
	}
}