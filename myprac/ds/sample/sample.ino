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
		int sok = in.sok(a1);
		// Serial.println(sok); // 4~50
		dispNum(sok);
		int i = clampv(map(sok, 4, 50, 0, 5), 0, 4);
		static const int l[5] = { B000, B001, B010, B100, B111 };
		led(l[i]);
	}
}
