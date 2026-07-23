// 32:34
// 合計時間 01:38:56 (課題3無し)

#define USE_TIMER3_ISR
#include "mono_con.h"

const int hello[7] = { 0x00, 0x00, 0x76, 0x79, 0x38, 0x38, 0x3F };
const int goodbye[10] = { 0x00, 0x00, 0x6F, 0x5C, 0x5C, 0x5E, 0x40, 0x7C, 0x6E, 0x79 };
int idxl = 0, idxr = 1;

int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int dispCnt = 1, nowCnt = 0;

word in, tc;

typedef enum {
	SET,
	HELLO,
	GOODBYE
} Status;
Status st;

void onedisp(int l, int r) {
	static int prel = 0x00, prer = 0x00;
	if (l != prel || r != prer) {
		prel = l, prer = r;
		disp(l, r);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (st == SET && sw == LOW && presw == HIGH) swps = true;
		if (st == SET && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);

	st = SET;

	disp(num[10], num[1]);
}

void loop() {

	switch (st) {
	case SET:
		if (swps) {
			swps = false;
			if (++dispCnt > 3) dispCnt = 1;
			disp(num[10], num[dispCnt]);
		}

		if (phps) {
			phps = false;
			if (tsw == LOW) st = HELLO;
			else st = GOODBYE;
			tc = 300;
			idxl = 0, idxr = 1;
			nowCnt = -1;
		}

		break;
	case HELLO:
		if (nowCnt < dispCnt) {
			if (tc > 300) {
				tc = 0;
				if (idxl == 0) nowCnt++;
				disp(hello[idxl], hello[idxr]);
				if (++idxl > 6) idxl = 0;
				if (++idxr > 6) idxr = 0;
			}
		} else {
			st = SET;
			disp(num[10], num[dispCnt]);
		}

		break;
	case GOODBYE:
		if (nowCnt < dispCnt) {
			if (tc > 300) {
				tc = 0;
				if (idxl == 0) nowCnt++;
				disp(goodbye[idxl], goodbye[idxr]);
				if (++idxl > 9) idxl = 0;
				if (++idxr > 9) idxr = 0;
			}
		} else {
			st = SET;
			disp(num[10], num[dispCnt]);
		}

		break;
	}
}