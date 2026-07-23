#include "mono_con.h"

void setup() {
	config_init();
	serial_init();

	disp(num[0], num[0]);
}

int count = 0;

void loop() {
	int ten = count / 10;
	int one = count % 10;

	disp(num[ten], num[one]);

	if (++count > 99) {
		count = 0;
	}

	delay(200);
}