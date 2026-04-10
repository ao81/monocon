// 9:48

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn[3] = { 0x50, 0x38, 0x00 };
const int dmspeed = 80;
const int hiEdge = 1023 - 200;

int ph, ypos;
int preph, preypos;
bool phps = false, yposps = false;

int idx = 0;
bool ok = true;

word in;

void DMcw() {
	analogWrite(FIN_PIN, dmspeed);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw() {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, dmspeed);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		ph = digitalRead(_USER_CON_3PIN);
		ypos = analogRead(A2);

		if (ph == LOW && preph == HIGH) phps = true;
		if (ypos > hiEdge && preypos <= hiEdge) yposps = true;

		preph = ph;
		preypos = ypos;
	}
}

void setup() {
	config_init();
	serial_init();

	ph = preph = digitalRead(_USER_CON_3PIN);
	ypos = analogRead(A2);

	disp(0x00, ptn[0]);
}

void loop() {
	if (phps) {
		phps = false;
		if (++idx > 2) idx = 0;
		disp(0x00, ptn[idx]);
	}

	if (yposps) {
		yposps = false;

		if (idx == 0 && ok) {
			ok = false;
			DMcw();
		} else if (idx == 1 && ok) {
			ok = false;
			DMccw();
		} else if (idx == 2) {
			ok = true;
			DMstop();
		}
	}
}