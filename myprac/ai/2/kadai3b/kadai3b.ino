// 10:32

#define USE_TIMER3_ISR
#include "mono_con.h"

const int seg[5] = { 0x76, 0x38, 0x3f, 0x73, 0x00 };
const int led[5] = { B001, B010, B100, B101, B000 };
int idx = 4;

int x, y;
int tsw, sw;
int presw;
bool swps = false;

int cnt = 0;

word in;

int getidx(int x, int y) {
	int dx = x - 512;
	int dy = y - 512;

	if (abs(dx) < 250 && abs(dy) < 250) return 4;

	if (abs(dy) >= abs(dx)) {
		return (dy < 0) ? 0 : 1;
	} else {
		return (dx < 0) ? 3 : 2;
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A1);
		y = analogRead(A2);
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	y = analogRead(A2);
	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (tsw == HIGH) {
		idx = getidx(x, y);
		if (idx != 4) {
			if (swps) {
				swps = false;
				if (++cnt > 99) cnt = 0;
			}
		}
	}

	if (swps) swps = false;

	disp(num[cnt % 10], seg[idx]);
	lm.color.GBR = led[idx];
	led_stepmotor(lm.b8);
}
