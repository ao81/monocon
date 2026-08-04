#define USE_TIMER3
#include "mono_con.h"

const uint8_t num2[17] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66,  // 0,1,2,3,4
	0x6d, 0x7d, 0x27, 0x7f, 0x6f,  // 5,6,7,8,9
	0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71, // A,b,c,d,E,F
	0x00                           // blank
};

const int dig[3] = { 10, 16, 2 };
int idx = 0;

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false;
bool r = true;

int n = 0;
int mode = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = !digitalRead(pin3);
	sw = presw = !digitalRead(pin4);
	ph = preph = digitalRead(pin5);
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
		n++;
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == HIGH) {
			if (++idx > 2) idx = 0;
		} else {
			tone(BZ_PIN, 2000, 1000);
		}
	}

	disp(num2[(n % (dig[idx] * dig[idx])) / dig[idx]], num2[(n % (dig[idx] * dig[idx])) % dig[idx]]);
}
