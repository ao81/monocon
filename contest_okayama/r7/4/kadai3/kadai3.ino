// 7:15

#define USE_TIMER3_ISR
#include "mono_con.h"

const int spdpara[5] = { -120, -80, 0, 80, 120 };
int idx = 2;
int x;

int ytoidx(int p) {
	return map(p, 0, 1023, 2, -3);
}

void cw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void rotate(int i) {
	if (i == 0) stop();
	else if (i < 0) ccw(abs(i));
	else cw(i);
}

void onedisp(int n) {
	static int pren = -3;
	if (pren != n) {
		pren = n;
		if (n < 0) disp(num[abs(n)], 0x40);
		else disp(num[n], 0x00);
	}
}

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		x = analogRead(A2);
	}
}

void setup() {
	config_init();
	serial_init();
	
	x = analogRead(A2);
}

void loop() {
	idx = ytoidx(x);
	onedisp(idx);
	rotate(spdpara[idx + 2]);
}