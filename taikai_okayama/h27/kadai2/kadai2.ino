// 19:05

#define USE_TIMER3_ISR
#include "mono_con.h"

const int wt1[6] = { 500, 500, 500, 500, 2000, 2000 };
const int wt2[6] = { 1000, 1000, 2000, 1000, 2000, 1000 };

int tsw, ph;
int dp[2] = { 0x00, 0x00 };
int idx = 0;
bool dowait = false;
bool first = false;

typedef enum {
	WAIT,
	TOUKA,
	SYADAN
} Status;
Status st = WAIT;

word in, tc;

void onedisp(int n[2]) {
	static int pren[2] = { 0x00, 0x00 };
	if (memcmp(n, pren, sizeof(pren)) != 0) {
		memcpy(pren, n, sizeof(pren));
		disp(n[0], n[1]);
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

	tsw = digitalRead(_USER_CON_5PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	switch (st) {
	case WAIT:
		if (tsw == HIGH) {
			if (ph == LOW) st = TOUKA;
			else st = SYADAN;
			first = true;
		}

		break;
	case TOUKA:
		if (first) {
			first = false;
			tc = 0;
			idx = 0;
			dp[1] = 0x00;
		}

		if (tc > wt1[idx]) {
			tc = 0;
			if (++idx >= 6) st = WAIT;
			else dp[1] ^= 0xff;
		}

		break;
	case SYADAN:
		if (first) {
			first = false;
			tc = 0;
			idx = 0;
			dp[0] = 0xff;
		}

		if (tc > wt2[idx]) {
			tc = 0;
			if (++idx >= 6) st = WAIT;
			else dp[0] ^= 0xff;
		}

		break;
	}

	onedisp(dp);
}