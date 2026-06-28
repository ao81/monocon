#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
}

Timer t;

void loop() {
	if (in.d(d2).fall) {
		led(0b100);
		t.start(1000);
	}
	if (t.done()) led(0);
}
