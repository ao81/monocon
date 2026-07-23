#define USE_TIMER3_ISR
#include "mono_con.h"

const int segptn[] = { 0x50, 0x38, 0x00 };
const int highEdge = 1023 - 200;

int ypos, preypos;
int ph, preph;
bool phps = false, swps= false;

int segIdx = 0;
int dmIdx = 0;
bool isccw = false;

word in, wait;

void DMcw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed) {
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
	} else if (isccw) {
		DMccw(80);
		isccw = false;
	}
}

void setup() {
	config_init();
	serial_init();

	ypos = preypos = analogRead(A2);
	ph = preph = digitalRead(_USER_CON_3PIN);

	DMcw(80);
	disp(0x00, segptn[0]);
}

void loop() {
	if (phps) {
		phps = false;
		if (++segIdx > 2) segIdx = 0;
		disp(0x00, segptn[segIdx]);
	}

	if (swps) {
		swps = false;
		if (++dmIdx > 2) dmIdx = 0;
		if (dmIdx == 0) DMcw(80);
		else if (dmIdx == 1) { wait = 300; isccw = true; }
		else DMstop();
	}
}