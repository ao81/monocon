#include "mono_con.h"

void setup() {
	config_init();
	serial_init();

	disp(num[0], num[0]);
}

int count = 99;
int dir = -1;

void loop() {
	int ten = count / 10;
	int one = count % 10;

	disp(num[ten], num[one]);

	count += dir;
	if (count <= 0 || count >= 99) {
		dir = -dir;
	}

	delay(200);
}