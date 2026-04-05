#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw1, sw2;
int pretsw, presw1, presw2;
bool sw1ps = false, sw2ps = false;

word in;

const int ledParam[4] = { B001, B100, B010, B111 };

typedef enum {
	IN_YEAR,
	IN_MONTH,
	IN_DAY,
	OUT_RESULT,
} Status;
Status st = IN_YEAR;

ISR(TIMER3_COMPA_vect) {
	if (++in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1ps = true;
		if (sw2 == LOW && presw2 == HIGH) sw2ps = true;

		presw1 = sw1;
		presw2 = sw2;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_4PIN);
	sw1 = presw1 = digitalRead(_USER_CON_5PIN);
	sw2 = presw2 = digitalRead(_USER_CON_3PIN);

	lm.color.GBR = ledParam[0];
}

void loop() {
	if (sw1ps) {
		sw1ps = false;
		if (st < OUT_RESULT) st = (Status)(st + 1);
		else st = IN_YEAR;
		lm.color.GBR = ledParam[(int)st];
	}

	switch (st) {
	case IN_YEAR:
		
		break;
	case IN_MONTH:

		break;
	case IN_DAY:

		break;
	case OUT_RESULT:

		break;
	}

	led_stepmotor(lm.b8);
}