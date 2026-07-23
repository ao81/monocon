// 52:24

// sw1をswとし、swをjs↓とする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int hi = 1023 - 200;

int dp[2] = { 0, 0 }, dpans[2] = { 0, 0 };
bool isplus = true;
bool first = true;
bool nolight = false;
int ans = 0;
int blinkcnt = 0;

int tsw, sw, ph, yjs;
int presw, preph, preyjs;
bool swps = false, phps = false, jsps = false;

word in, tc;

typedef enum {
	SET,
	RES
} Status;
Status st = SET;

void onedisp(int n[2], bool minus = false, bool no = false) {
	static int pren[2] = { 10, 10 };
	static bool preminus = false, preno = false;
	if (memcmp(n, pren, sizeof(pren)) != 0 || minus != preminus || no != preno) {
		memcpy(pren, n, sizeof(pren));
		preminus = minus;
		preno = no;
		if (no) disp(0x00, 0x00);
		else if (minus) disp(0x40, num[n[1]]);
		else disp(num[n[0]], num[n[1]]);
	}
}

void resetdisp() {
	int reset[2] = { 10, 10 };
	onedisp(reset);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
		yjs = analogRead(A2);

		if (st == SET && sw == LOW && presw == HIGH) swps = true;
		if (st == SET && ph == HIGH && preph == LOW) phps = true;
		if (st == SET && yjs > hi && preyjs <= hi) jsps = true;

		presw = sw;
		preph = ph;
		preyjs = yjs;
	}

	if (st == RES) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
	yjs = preyjs = analogRead(A2);
}

void loop() {
	switch (st) {
	case SET:
		if (first) {
			first = false;
			resetdisp();
		}

		if (swps) {
			swps = false;
			if (tsw == HIGH) {
				if (++dp[0] > 9) dp[0] = 0;
			} else {
				if (++dp[1] > 9) dp[1] = 0;
			}
		}

		if (phps) {
			phps = false;
			isplus = !isplus;
		}

		if (jsps) {
			jsps = false;
			st = RES;
			first = true;
		}

		onedisp(dp);
		break;
	case RES:
		if (first) {
			first = false;
			nolight = false;
			tc = 0;
			blinkcnt = 1;
			if (isplus) ans = dp[0] + dp[1];
			else ans = dp[0] - dp[1];
			dpans[0] = ans / 10;
			dpans[1] = abs(ans % 10);
		}

		if (blinkcnt < 11) {
			if (tc > 500) {
				tc = 0;
				if (blinkcnt % 2 == 1) {
					dpans[0] = dpans[1] = 10;
					nolight = true;
				} else {
					dpans[0] = ans / 10;
					dpans[1] = abs(ans % 10);
					nolight = false;
				}
				blinkcnt++;
			}
		} else {
			st = SET;
			first = true;
		}

		onedisp(dpans, ans < 0, nolight);
		break;
	}

	if (isplus) lm.color.GBR = B000;
	else lm.color.GBR = B111;
	led_stepmotor(lm.b8);
}