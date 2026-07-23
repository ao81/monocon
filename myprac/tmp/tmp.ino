#include "monocon_chuugoku.h"

void isr() {}

void setup() {
	begin();
	Serial.begin(9600);
}

void loop() {
	Serial.print(digitalRead(p1));
	Serial.println(digitalRead(p2));
	delay(80);
}
