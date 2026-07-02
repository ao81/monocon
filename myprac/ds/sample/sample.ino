#include "monocon_chuugoku.h"

word tc;
void isr() { tc++; }

void setup() {
	begin();
	Serial.begin(9600);
}

void loop() {
	if (tc >= 5000) {
		tc = 0;
		int s = sok(a3);
		// Serial.println(sok); // 4~50
		dispNum(s);
	}

	static bool t1 = false, t2 = false;
	if (in.d(d2).htol) t1 = !t1;
	if (in.d(d1).ltoh) t2 = !t2;
	int l = 0b000;
	if (t2) l |= 0b001;
	if (t1) l |= 0b010;
	if (in.d(d3)) l |= 0b100;
	led(l);
}
