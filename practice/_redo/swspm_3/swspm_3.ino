#define USE_TIMER3_ISR
#include "mono_con.h"

const int SPM_SPEED = 20;

int tsw, sw;
int presw;
bool swps = false;
bool isCw = true;

int phase = 0, ex = 0;

word in, spm;

typedef enum {
	WAIT,
	ROLL
} Status;
Status st;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw = digitalRead(_USER_CON_5PIN);

		if (st == WAIT && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (st == ROLL) spm++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_4PIN);
	sw = presw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	switch (st) {
	case WAIT:
		if (swps) {
			swps = false;

			isCw = (tsw == HIGH);

			ex = 0;
			spm = SPM_SPEED + 1;

			st = ROLL;
		}

		break;
	case ROLL:
		if (ex <= 120) {
			if (spm > SPM_SPEED) {
				spm = 0;
				lm.color.SM = stepm_init(phase);
				led_stepmotor(lm.b8);
				if (isCw) { if (++phase > 3) phase = 0; } else { if (--phase < 0) phase = 3; }
				ex++;
			}
		} else {
			st = WAIT;
		}

		break;
	}
}