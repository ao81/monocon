#include "mono_con.h"

void setup() {
	config_init();
	serial_init();
}

int value = 0;

void loop() {
	disp(num[value], num[value]);
	delay(1000);
	if (++value > 9) value = 0;
}