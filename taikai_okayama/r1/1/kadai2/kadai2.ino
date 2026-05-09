#define USE_TIMER3_ISR
#include "mono_con.h"

const int segptn[] = { 0x50, 0x38, 0x00 };
const int highEdge = 1023 - 200;

int ypos, preypos;
int ph, preph;
bool phps = false, swps = false;

int idx = 0, preidx = 0;
bool iscw = false, isccw = false;

word in, wait;

void DMcw(int speed = 80) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed = 80) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		ypos = analogRead(A2);
		ph = digitalRead(_USER_CON_3PIN);

		if (ypos > highEdge && preypos <= highEdge) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		preypos = ypos;
		preph = ph;
	}

	if (wait > 0) {
		wait--;
		DMstop();
	} else if (iscw) {
		DMcw();
		iscw = false;
	} else if (isccw) {
		DMccw();
		isccw = false;
	}
}

void setup() {
	config_init();
	serial_init();

	ypos = preypos = analogRead(A2);
	ph = preph = digitalRead(_USER_CON_3PIN);

	DMcw();
	disp(0x00, segptn[0]);
}

void loop() {
	if (phps) {
		phps = false;
		if (++idx > 2) idx = 0;
		disp(0x00, segptn[idx]);
	}

	if (swps) {
		swps = false;

		if (idx == 0) {
			if (preidx == 1) {
				wait = 150;
				iscw = true;
			} else {
				DMcw();
			}
		} else if (idx == 1) {
			if (preidx == 0) {
				wait = 150;
				isccw = true;
			} else {
				DMccw();
			}
		} else DMstop();
		preidx = idx;
	}
}