// 7:49

#define USE_TIMER3_ISR
#include "mono_con.h"

const int wait[6] = { 500, 500, 500, 500, 2000, 2000 };
int idx = 0;

int tsw, ph, pretsw;

word in = 0, tc = 0;

typedef enum {
	ON,
	OFF
} Status;
Status st = OFF;

bool ison = false, left = false;
void onedisp(bool on, bool l = true) {
	static bool preon = false, prel = true;
	if (preon != on || prel != l) {
		preon = on, prel = l;
		if (on) {
			if (l) disp(0xff, 0x00);
			else disp(0x00, 0xff);
		} else disp(0x00, 0x00);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
		ph = digitalRead(_USER_CON_3PIN);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();
	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			st = ON;
			tc = 0;
			idx = 0;
			ison = true;
		}

	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			st = OFF;
		}
	}

	if (st == ON) {
		if (tc > wait[idx]) {
			tc = 0;
			ison = (idx % 2 == 1);
			if (++idx > 5) idx = 0;
		}

		if (ph == LOW) left = false;
		else left = true;

	} else if (st == OFF) {
		ison = false;
		left = false;
	}

	onedisp(ison, left);
}