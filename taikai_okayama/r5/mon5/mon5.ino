#define USE_TIMER3_ISR
#include "mono_con.h"

int TSW = HIGH, SW = HIGH, PH = HIGH;
int preTSW = HIGH, preSW = HIGH, prePH = HIGH;

int count = 0, res = 0;

word in, cd;

typedef enum {
	WAIT,
	COUNT,
	RESULT,
} GameStates;
GameStates gs = WAIT, pregs = WAIT;

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		TSW = digitalRead(19);
		SW = digitalRead(18);
		PH = digitalRead(17);
	}

	in++;

	if (gs == COUNT) cd++;
}

void setup() {
	config_init();
	serial_init();

	disp(num[0], num[0]);
}

void loop() {
	GameStates gs_ = gs;

	if (SW == LOW && preSW == HIGH) {
		gs = WAIT;
	}

	if (gs == WAIT && PH == HIGH && prePH == LOW) {
		count++;
	}

	switch (gs) {
	case WAIT:
		if (gs != pregs) {
			count = 0;
			disp(num[0], num[0]);
			cd = 0;
			res = 0;
		}

		if (PH == HIGH && prePH == LOW) {
			count = 1;
			gs = COUNT;
		}

		break;
	case COUNT:
		if (gs != pregs) disp(num[count / 10], num[count % 10]);

		if (PH == HIGH && prePH == LOW) {
			count++;
			disp(num[count / 10], num[count % 10]);
		}
		if (count >= 20) gs = RESULT;

		break;

	case RESULT:
		if (gs != pregs) {
			res = cd / 100;
			if (res > 99) res = 99;
			disp(num[res / 10] | 0x80, num[res % 10]);
		}

		break;
	}

	preTSW = TSW;
	preSW = SW;
	prePH = PH;
	pregs = gs_;
}