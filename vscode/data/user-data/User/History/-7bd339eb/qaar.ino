#define USE_TIMER3_ISR
#include "mono_con.h"


const int bz[8] = { 262, 294, 330, 349, 392, 440, 494, 523 };
enum { c4, d4, e4, f4, g4, a4, b4, c5 };

int tsw, sw, x;
int pretsw, presw;
bool swps = false;

int round = 1;

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

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	x = analogRead(A1);

	pretsw = !tsw;
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == LOW) {
			st = INIT;
			lm.color.GBR = B000;
			round = 1;
		} else {
			st = GAME;
			lm.color.GBR = B100;
		}
	}

	switch (st) {
	case INIT:
		if (swps) {
			swps = false;
			st = GAME;
		}

		break;
	case GAME:

		break;
	case RES:

		break;
	}

	led_stepmotor(lm.b8);
}
