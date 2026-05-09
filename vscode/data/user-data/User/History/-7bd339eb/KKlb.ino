#define USE_TIMER3_ISR
#include "mono_con.h"

const int bz[8] = { 262, 294, 330, 349, 392, 440, 494, 523 };
int tsw, sw, x;
int pretsw;

word in;

typedef enum {
	INIT,
	GAME,
	RES,
} Status;
Status st;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		x = analogRead(A1);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	x = analogRead(A1);
}

void loop() {

	if (tsw == LOW) {
		st = INIT;
	} else {
		st = GAME;
	}

	switch (st) {
	case INIT:

		break;
	case GAME:

		break;
	case RES:

		break;
	}
}
