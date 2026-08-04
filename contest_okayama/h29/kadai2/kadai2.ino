// 13:08

/** 点滅速度の3段階の調整を
 *  ロータリーエンコーダから
 *  tswをトグルするたびに切り替わるように変更
 */

#define USE_TIMER3_ISR
#include "mono_con.h"

const int waitms[3] = { 200, 600, 1000 };
int waitidx = 0;
const int ledParam[8] = { B000, B001, B100, B101, B010, B011, B110, B111 };
int ledidx = 0;

bool ismove = true;

int sw, tsw;
int presw, pretsw;
bool swps = false, tswps = false;

word in, tc;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		tsw = digitalRead(_USER_CON_5PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (tsw != pretsw) tswps = true;

		presw = sw;
		pretsw = tsw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);
	tsw = pretsw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (swps) {
		swps = false;
		ismove = !ismove;
	}

	if (tswps) {
		tswps = false;
		if (++waitidx > 2) waitidx = 0;
	}

	if (ismove) {
		if (tc > waitms[waitidx]) {
			tc = 0;
			if (++ledidx > 7) ledidx = 0;
			lm.color.GBR = ledParam[ledidx];
		}
	}

	led_stepmotor(lm.b8);
}