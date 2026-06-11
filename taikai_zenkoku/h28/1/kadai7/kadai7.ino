#define USE_TIMER3
#include "mono_con.h"

const int per[5] = { 0, 25, 50, 75, 100 };
int idx = 0;

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false;
bool r = true;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}

	static int acc = 0;
	acc += per[idx];
	if (acc >= 100) {
		acc -= 100;
		lm.led(B111).flush();
	} else lm.led(B000).flush();
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = !digitalRead(pin3);
	sw = presw = !digitalRead(pin4);
	ph = preph = digitalRead(pin5);

	disp(0x00, 0x00);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = !digitalRead(pin3);
		sw = !digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (swps) {
		swps = false;
		if (++idx > 4) idx = 0;

		if (idx == 0) disp(0x00, 0x00);
		else if (idx == 4) disp(0x5c, 0x54);
		else disp(num[per[idx] / 10], num[per[idx] % 10]);
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == HIGH) {
			idx = 0;
			disp(0x00, 0x00);
		}
	}
}
