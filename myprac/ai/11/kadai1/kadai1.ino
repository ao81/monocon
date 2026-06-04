// 13:52

#define USE_TIMER3
#include "mono_con.h"

const int notes[8] = { 262, 294, 330, 349, 392, 440, 494, 523 };

int x, y;
int tsw, sw, ph;
int pretsw, presw;
bool swps = false;
bool input = true;
bool e = false;
word cd = 0;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
	}
	if (cd > 0) {
		cd--;
		if (cd == 0) {
			lm.led(B000).flush();
			e = true;
		}
	}
}

void setup() {
	config_init();
	serial_init();
	sw = presw = digitalRead(pin4);
}

void loop() {
	if (input) {
		input = false;
		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	if (e) {
		e = false;
		noTone(BZ_PIN);
	}

	if (cd <= 0) {
		int i = getdir(x, y);
		if (i == -1) {
			noTone(BZ_PIN);
			disp(0x00, 0x00);
			lm.led(B000);
		} else {
			if (tsw == LOW) tone(BZ_PIN, notes[i]);
			else tone(BZ_PIN, notes[i] * 2);
			disp(0x00, num[i + 1]);
			lm.led(B111);
		}
		lm.flush();
	} else {
		lm.led(B100).flush();
		if (tsw == LOW) tone(BZ_PIN, notes[0]);
		else tone(BZ_PIN, notes[0] * 2);
		disp(0x00, 0x00);
	}

	if (swps) {
		swps = false;
		cd = 500;
	}
}
