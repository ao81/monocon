// 17:29

#define USE_TIMER3_ISR
#include "mono_con.h"

const int speed[16] = { 0, 100, 94, 88, 82, 76, 70, 64, 58, 52, 46, 40, 34, 28, 22, 16 };
int speedidx = 0;

int phase = 0;

int sw, tsw;
int presw;
bool swps = false;

word in, spm;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		tsw = digitalRead(_USER_CON_5PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	spm++;
}

void setup() {
	config_init();
	serial_init();

	disp(0x00, num[0]);
}

void loop() {
	if (swps) {
		swps = false;
		if (++speedidx > 15) speedidx = 0;
		disp(0x00, num[speedidx]);
		spm = 0;
	}

	if (speedidx != 0) {
		if (spm > speed[speedidx]) {
			spm = 0;
			lm.color.SM = stepm_init(phase);
			if (tsw == HIGH) {
				if (++phase > 3) phase = 0;
			} else {
				if (--phase < 0) phase = 3;
			}
			led_stepmotor(lm.b8);
		}

	} else {
		lm.color.SM = stepm_init(4);
		led_stepmotor(lm.b8);
	}
}