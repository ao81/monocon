// 5:27

// xpos←,→ を sw1,sw2 とする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int lo = 0 + 200;
const int hi = 1023 - 200;

int dp[2] = { 0x00, 0x00 };

int tsw, xpos;

word in;

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
		xpos = analogRead(A1);
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (tsw == HIGH) dp[0] = dp[1] = 0xFF;
	else if (tsw == LOW) dp[0] = dp[1] = 0x00;
	if (xpos > hi) dp[0] ^= 0xFF;
	if (xpos < lo) dp[1] ^= 0xFF;
	onedisp(dp);
}