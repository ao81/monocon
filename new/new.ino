#define useir
#include "monocon_chuugoku.h"

void setup() {
	begin();
}

int dir = 1;
int b = 0;
int n = 0;

void ir() {
	n++;
}

void loop() {
	if (n >= 100) {
		n = 0;
		b += dir;
		if (b == 100) dir = -1;
		if (b == 0) dir = 1;
	}
	led(R, b);
	dp.s(bye, 100, 50, 30);
}
