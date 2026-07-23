#define USE_TIMER3_ISR
#include "mono_con.h"

const int lowEdge = 0 + 200;
const int dispNengou[4] = { 0x07, 0x6D, 0x76, 0x50 }; // T, S, H, R

int sw, ypos, ph;
int presw, preypos, preph;
bool swps = false, yposps = false, phps = false;

int phase = 0, dispcnt = 0;

int count = 0;

word in, tc;

typedef enum {
	INPT,
	DISP,
} Status;
Status st = INPT, prest = INPT;

typedef struct {
	int nengou_idx;
	int wareki;
} Data;
Data dt = { 0, 0 };

void onedisp(int n, bool force = false) {
	static int pren = -1;
	if (force || pren != n) {
		pren = n;
		disp(num[n / 10], num[n % 10]);
	}
}

int getSeireki(int n) {
	if (n >= 21 && n <= 99) {
		return 1900 + n;
	} else {
		return 2000 + n;
	}
}

Data getData(int n) {
	Data res = { 0, 0 };
	int sr = getSeireki(n);

	if (sr >= 1921 && sr <= 1925) {
		sr -= 1911; res.nengou_idx = 0;
	} else if (sr >= 1926 && sr <= 1988) {
		sr -= 1925; res.nengou_idx = 1;
	} else if (sr >= 1989 && sr <= 2018) {
		sr -= 1988; res.nengou_idx = 2;
	} else {
		sr -= 2018; res.nengou_idx = 3;
	}

	res.wareki = sr;
	return res;
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ypos = analogRead(A2);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ypos < lowEdge && preypos >= lowEdge) yposps = true;
		if (st == INPT && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preypos = ypos;
		preph = ph;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = digitalRead(_USER_CON_4PIN);
	ypos = analogRead(A2);

	disp(num[0], num[0]);
}

void loop() {
	switch (st) {
	case INPT:
		if (st != prest) prest = st;


		if (swps || yposps) {
			swps = yposps = false;
			tc = 500;
		}

		if (sw == LOW) {
			if (tc > 500) {
				tc = 0;
				if (++count > 99) count = 0;
			}
		} else if (ypos < lowEdge) {
			if (tc > 500) {
				tc = 0;
				if (--count < 0) count = 99;
			}
		}

		if (phps) {
			phps = false;
			st = DISP;
		}

		onedisp(count);

		break;
	case DISP:
		if (st != prest) {
			prest = st;
			dt = getData(count);
			tc = 1000;
			phase = 0;
			dispcnt = 0;
		}

		if (dispcnt <= 6) {
			if (tc > 1000) {
				tc = 0;
				if (phase == 0) {
					disp(0x00, dispNengou[dt.nengou_idx]);
				} else if (phase == 1) {
					onedisp(dt.wareki, true);
				}
				if (++phase > 1) phase = 0;
				dispcnt++;
			}
		} else {
			st = INPT;
		}

		break;
	}
}