// 5:25
// sw2 -> ph

#define USE_TIMER3_ISR
#include "mono_con.h"

const int spd[5] = { 130, 80, 0, 80, 130 };

int sw, ph;
int presw, preph;
bool swps = false, phps = false;

int val = 0;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == LOW && preph == HIGH) phps = true;

		presw = sw;
		preph = ph;
	}
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

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (swps) {
		swps = false;
		if (val > -2) val--;
	}

	if (phps) {
		phps = false;
		if (val < 2) val++;
	}

	int idx = val + 2;
	if (val > 0) cw(spd[idx]);
	else if (val < 0) ccw(spd[idx]);
	else stop();


	if (val >= 0) disp(0x00, num[val]);
	else disp(0x40, num[abs(val)]);
}
