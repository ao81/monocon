#define USE_TIMER3_ISR
#include "mono_con.h"

const int y_pin = A2;
const int D3 = 19; // TSW

int y_pos = 0;
int TSW = 0;

int value = 0, prev_value = 0;

word in;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		y_pos = analogRead(y_pin);
		TSW = digitalRead(D3);
	}

	in++;
}

void setup() {
	config_init();
	serial_init();
}

void DMcw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void loop() {
	if (TSW == HIGH) {
		// on

		if (y_pos < 200) {
			// 上に倒した
			DMcw(100);
			value = 2;

		} else if (y_pos > 800) {
			// 下に倒した
			DMccw(100);
			value = -2;

		} else {
			// 倒していない
			DMstop();
			value = 0;
		}

	} else {
		// off

		if (y_pos < 200) {
			// 上に倒した
			DMcw(60);
			value = 1;

		} else if (y_pos > 800) {
			// 下に倒した
			DMccw(60);
			value = -1;

		} else {
			// 倒していない
			DMstop();
			value = 0;
		}
	}

	if (value != prev_value) {
		if (value < 0) {
			disp(0x40, num[abs(value)]);
		} else {
			disp(num[10], num[value]);
		}
	}

	prev_value = value;
}