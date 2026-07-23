#include "mono_con.h"

void setup() {
	config_init();
	serial_init();
}

void loop() {
	disp(num[8], num[8]);
	delay(500);
	disp(num[10], num[10]);
	delay(500);
}