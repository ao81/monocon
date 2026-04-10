#define USE_TIMER3_ISR
#include "mono_con.h"

const int lowEdge =  0 + 200;
const int highEdge = 1023 - 200;

int ypos;
int sw;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		ypos = analogRead(A2);
		sw = digitalRead(_USER_CON_4PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	ypos = analogRead(A2);
	sw = digitalRead(_USER_CON_4PIN);
}

const int color[] = { B001, B101, B100 };
const int ptn[] = { 0x08, 0x40, 0x01 };
int prev_ptn = 0;

void loop() {
	int dispcolor = B000;
	int dispptn = 0x00;

	if (sw == LOW) { // SW1
		dispcolor |= color[0];
		dispptn |= ptn[0];
	}

	if (ypos < lowEdge) { // SW2
		dispcolor |= color[1];
		dispptn |= ptn[1];
	}

	if (ypos > highEdge) { // SW3
		dispcolor |= color[2];
		dispptn |= ptn[2];
	}

	if (prev_ptn != dispptn) {
		prev_ptn = dispptn;
		disp(0x00, dispptn);
	}

	lm.color.GBR = dispcolor;
	led_stepmotor(lm.b8);
}