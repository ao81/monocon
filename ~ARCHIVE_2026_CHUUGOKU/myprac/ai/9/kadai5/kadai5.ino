#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin3);
		sw = digitalRead(pin4);
		ph = digitalRead(pin5);
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin3);
	sw = digitalRead(pin4);
	ph = digitalRead(pin5);
}

void loop() {

}
