#define USE_TIMER3_ISR
#include "mono_con.h"

word sTime;
word spm;
word tgl;

void setup(void) {
	config_init();
	serial_init();
}

ISR(TIMER3_COMPA_vect) {
	if (sTime > 5) {
		sTime = 0;
	}

	sTime++;
	spm++;
	tgl++;
}

int phase = 0;
int status = 0;

void loop(void) {
	if (tgl > 5000) {
		tgl = 0;
		if (++status > 3) status = 0;
	}

	if (spm > 10) {
		if (status == 0) {
			spm = 0;
			lm.color.SM = stepm_init(phase);
			if (++phase > 3) phase = 0;

		} else if (status == 2) {
			spm = 3;
			lm.color.SM = stepm_init(phase);
			if (--phase < 0) phase = 3;

		} else {
			lm.color.SM = stepm_init(4);

		}

		led_stepmotor(lm.b8);
	}
}