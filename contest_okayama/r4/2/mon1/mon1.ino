#define USE_TIMER3_ISR
#include "mono_con.h"

const int color[3] = { B100, B001, B000 };
const int delayms = 300;

int tsw;
int pretsw;

word in;

void blinking(int cnt, int coloridx) {
	for (int i = 0; i < cnt; i++) {
		lm.color.GBR = color[coloridx];
		led_stepmotor(lm.b8);
		delay(delayms);
		
		lm.color.GBR = color[2];
		led_stepmotor(lm.b8);
		delay(delayms);
	}
}

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_4PIN);
	}
	in++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	static int count = 0;

	if (tsw != pretsw) {
		pretsw = tsw;
		blinking(++count, count % 2);
	}
}