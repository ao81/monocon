// 23:55

#define USE_TIMER3_ISR
#include "mono_con.h"

const int loedge = 0 + 200;
const int hiedge = 1023 - 200;

const int speed[7] = { 140, 110, 80, 0, 80, 110, 140 };
int idx = 3;

int sw, xpos;
int presw, prexpos, xposps = 0;
bool swps = false;

bool isstop = true;

word in;

void dm(int speed, int di) {
	if (di == 0) {
		digitalWrite(FIN_PIN, HIGH);
		digitalWrite(RIN_PIN, HIGH);
	} else {
		analogWrite(FIN_PIN, di > 0 ? speed : 0);
		analogWrite(RIN_PIN, di < 0 ? speed : 0);
	}
}

void onedisp(int n) {
	static int pre_n = 0;
	if (n != pre_n) {
		pre_n = n;
		if (n >= 0) disp(0x00, num[n]);
		else disp(0x40, num[abs(n)]);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		xpos = analogRead(A1);

		if (sw == LOW && presw == HIGH) swps = true;
		if (xpos > hiedge && prexpos <= hiedge) xposps = 1;
		if (xpos < loedge && prexpos >= loedge) xposps = -1;

		presw = sw;
		prexpos = xpos;
	}
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);
	xpos = prexpos = analogRead(A1);

	disp(0x00, num[0]);
}

void loop() {
	if (swps) {
		swps = false;
		isstop = !isstop;
	}

	if (xposps != 0) {
		if (xposps > 0) {
			if (idx < 6) idx++;
		} else {
			if (idx > 0) idx--;
		}
		xposps = 0;
	}

	if (!isstop) {
		dm(speed[idx], (idx - 3 == 0) ? 0 : (idx - 3 > 0) ? 1 : -1);
	} else {
		dm(0, 0);
	}

	onedisp(idx - 3);
}