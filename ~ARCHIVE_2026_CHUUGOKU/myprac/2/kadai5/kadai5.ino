// 15:24

#define USE_TIMER3_ISR
#include "mono_con.h"

int x, angle;
int move = 0;
int phase = 0;
bool ismoving = false;

word in, tc;

int getangle(int pos) {
	return map(pos, 0, 1023, 0, 60);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		angle = getangle(x);
	}

	if (ismoving) tc++;
	else tc = 20;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
}


void loop() {
	static int preangle = -1;
	angle = getangle(x);

	if (angle != preangle) {
		move += angle - preangle;
		preangle = angle;
	}

	if (move > 0) {
		ismoving = true;
		if (tc > 20) {
			tc = 0;
			if (++phase > 3) phase = 0;
			lm.color.SM = stepm_init(phase);
			led_stepmotor(lm.b8);
			move--;
		}
	} else if (move < 0) {
		ismoving = true;
		if (tc > 20) {
			tc = 0;
			if (--phase < 0) phase = 3;
			lm.color.SM = stepm_init(phase);
			led_stepmotor(lm.b8);
			move++;
		}
	} else {
		ismoving = false;
	}
}