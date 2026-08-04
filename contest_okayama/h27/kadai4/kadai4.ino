// 18:27

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int dp[2] = { 0, 0 };

word in, tc;

typedef enum {
	TENMETU,
	KENSYUTU,
	BLINK
} Status;
Status st = TENMETU, prest = TENMETU;

void onedisp(int n[2], bool l, bool force = false) {
	static int pren[2] = { 10, 10 };
	if (memcmp(n, pren, sizeof(pren)) != 0 || force) {
		memcpy(pren, n, sizeof(pren));
		if (l) disp(num[n[0]], 0x00);
		else disp(0x00, num[n[1]]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (st == TENMETU && sw == LOW && presw == HIGH) swps = true;
		if (st == KENSYUTU && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (st == BLINK) tc++;
}

void setup() {
	config_init();
	serial_init();
	disp(num[0], num[10]);

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == LOW) st = TENMETU;
	else if (st == BLINK) st = BLINK;
	else st = KENSYUTU;

	switch (st) {
	case TENMETU:
		if (st != prest) {
			prest = st;
			onedisp(dp, true, true);
			lm.color.GBR = B000;
			led_stepmotor(lm.b8);
		}

		if (swps) {
			swps = false;
			if (++dp[0] > 9) dp[0] = 0;
		}

		onedisp(dp, true);
		break;
	case KENSYUTU:
		if (st != prest) {
			prest = st;
			onedisp(dp, false, true);
			lm.color.GBR = B000;
			led_stepmotor(lm.b8);
		}

		if (phps) {
			phps = false;
			if (++dp[1] > 9) dp[1] = 0;
		}

		if (dp[0] == dp[1]) st = BLINK;

		onedisp(dp, false);
		break;
	case BLINK:
		if (st != prest) {
			prest = st;
			tc = 0;
			lm.color.GBR = B111;
		}

		if (tc > 500) {
			tc = 0;
			lm.color.GBR ^= B111;
		}

		led_stepmotor(lm.b8);
		break;
	}
}