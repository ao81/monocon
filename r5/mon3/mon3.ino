#define USE_TIMER3_ISR
#include "mono_con.h"

#define push(a, b) (a == LOW && b == HIGH)

const int ptn1[5][2] = {
	{ 0x21, 0x00 },
	{ 0x42, 0x00 },
	{ 0x00, 0x50 },
	{ 0x00, 0x0c },
	{ 0x00, 0x00 },
};

const int ptn2[5][2] = {
	{ 0x30, 0x00 },
	{ 0x01, 0x01 },
	{ 0x00, 0x06 },
	{ 0x08, 0x08 },
	{ 0x00, 0x00 },
};

enum {
	PTN1,
	PTN2,
	PTN3,
	PTN4,
	INIT,
};
int st = INIT, prest = INIT;

int TSW = HIGH, SW1 = HIGH, SW2 = HIGH;
int preTSW = HIGH, preSW1 = HIGH, preSW2 = HIGH;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		TSW = digitalRead(18);
		SW1 = digitalRead(19);
		SW2 = digitalRead(17);
	}

	in++;
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	int _st = st;

	if (TSW == LOW) {
		if (TSW != preTSW) st = INIT;

		if (st != prest) disp(ptn1[st][0], ptn1[st][1]);

		if (st == INIT) {
			if (push(SW1, preSW1)) st = PTN1;
			if (push(SW2, preSW2)) st = PTN4;
		} else {
			if (push(SW1, preSW1)) if (++st > 3) st = 0;
			if (push(SW2, preSW2)) if (--st < 0) st = 3;
		}

	} else {
		if (TSW != preTSW) st = INIT;

		if (st != prest) disp(ptn2[st][0], ptn2[st][1]);

		if (st == INIT) {
			if (push(SW1, preSW1)) st = PTN1;
			if (push(SW2, preSW2)) st = PTN4;
		} else {
			if (push(SW1, preSW1)) if (++st > 3) st = 0;
			if (push(SW2, preSW2)) if (--st < 0) st = 3;
		}
	}

	prest = _st;
	preTSW = TSW;
	preSW1 = SW1;
	preSW2 = SW2;
}