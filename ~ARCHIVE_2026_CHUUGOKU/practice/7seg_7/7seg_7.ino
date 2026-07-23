#include "mono_con.h"

int ac[2] = { 0x77, 0x39 };

void setup() {
	config_init();
	serial_init();
	disp(ac[0], ac[1]);
}

void loop() {
}