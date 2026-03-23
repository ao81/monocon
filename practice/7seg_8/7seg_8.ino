#include "mono_con.h"

int newnum[16] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66,  		// 0, 1, 2, 3, 4
                	0x6d, 0x7d, 0x27, 0x7f, 0x6f,  			// 5, 6, 7, 8, 9
									0x77, 0x7f, 0x39, 0x3f, 0x79, 0x71,	// A, B, C, D, E, F
                };

void setup() {
	config_init();
	serial_init();
}

int count = 0;

void loop() {
	int ten = count / 16;
	int one = count % 16;

	disp(newnum[ten], newnum[one]);

	if (++count > 255) {
		count = 0;
	}

	delay(100);
}