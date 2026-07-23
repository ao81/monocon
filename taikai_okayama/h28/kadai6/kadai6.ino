#define USE_TIMER3_ISR
#include "mono_con.h"

const int hello[5] = { 0x76, 0x79, 0x38, 0x38, 0x3F };
const int goodbye[8] = { 0x6F, 0x5C, 0x5C, 0x5E, 0x40, 0x7C, 0x6E, 0x79 };
int index = 0;

int tsw, sw, ph;
int presw, preph;

word in;

typedef enum {
	SET,
	HELLO,
	GOODBYE
} Status;
Status st;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);

	st = SET;
}

void loop() {

	switch (st) {
	case SET:
		

		break;
	case HELLO:


		break;
	case GOODBYE:


		break;
	}
}