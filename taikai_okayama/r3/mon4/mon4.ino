// ロータリーエンコーダを用いる回転数の処理は無い

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw1, sw2;
int pretsw, presw1, presw2;
bool sw1ps = false, sw2ps = false;

int rotateidx = 0;
int phase = 0;

word in, spm, led;

typedef enum {
	STOP,
	ROTATE,
	PAUSE,
} States;
States st = STOP;

void SMcw() {
	lm.color.SM = stepm_init(phase);
	if (++phase > 3) phase = 0;
}

void SMccw() {
	lm.color.SM = stepm_init(phase);
	if (--phase < 0) phase = 3;
}

void SMstop() {
	lm.color.SM = stepm_init(4);
}

void SMpause() {
	lm.color.SM = stepm_init(phase);
}

void (*rotate[])() = { SMcw, SMccw };

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1ps = true;
		if (sw2 == LOW && presw2 == HIGH) sw2ps = true;

		presw1 = sw1;
		presw2 = sw2;
	}

	in++;
	spm++;
	led++;
}

void setup() {
	config_init();
	serial_init();

	disp(num[0], num[0]);
	SMstop();

	tsw = digitalRead(_USER_CON_4PIN);
	sw1 = digitalRead(_USER_CON_5PIN);
	sw2 = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		rotateidx = !rotateidx;
	}

	switch (st) {
	case STOP:
		SMstop();
		lm.color.GBR = B100;

		if (sw1ps) {
			spm = 40;
			sw1ps = false;
			st = ROTATE;
		}
		if (sw2ps) sw2ps = false;

		break;
	case ROTATE:
		if (spm > 40) {
			spm = 0;
			rotate[rotateidx]();
		}

		lm.color.GBR = B001;

		if (sw1ps) sw1ps = false;
		if (sw2ps) {
			sw2ps = false;
			led = 0;
			st = PAUSE;
		}

		break;
	case PAUSE:
		SMpause();
		if (led < 250) {
			lm.color.GBR = B101;
		} else if (led < 500) {
			lm.color.GBR = B000;
		} else {
			led = 0;
		}

		if (sw1ps) {
			spm = 40;
			sw1ps = false;
			st = ROTATE;
		}
		if (sw2ps) sw2ps = false;

		break;
	}

	led_stepmotor(lm.b8);
}