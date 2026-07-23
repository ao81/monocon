#include "monocon_chuugoku.h"

int sw, presw;
bool swps = false;
bool r = true;

void isr() {
	in.fenc(p1, p2);

	static word i = 0;
	if (i++ > 50) {
		i = 0;
		r = true;
	}
}

void setup() {
	begin();
	sw = presw = digitalRead(p4);
}

void loop() {
	if (r) {
		r = false;
		sw = digitalRead(p4);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}

	if (swps) {
		swps = false;
		c = 0;
	}

	disp(seg[c / 10], seg[c % 10]);
}
