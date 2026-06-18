#include "mono_con.h"

void setup() {
	config_init();
	serial_init();
}

int count = 0;
int digit;

void loop() {
	if (count == 0) {
		digit = 10;
	} else if (count % 2 == 1) {
		digit = 1;
	} else {
		digit = 2;
	}

	disp(num[count], num[digit]);

	if (++count > 9) {
		count = 0;
	}

	delay(1000);
}