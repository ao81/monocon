#define USE_TIMER3_ISR
#include "mono_con.h"

const int ptn1[5][2] = {
	{ 0x30, 0x00 },
	{ 0x06, 0x00 },
	{ 0x00, 0x30 },
	{ 0x00, 0x06 },
	{ 0x00, 0x00 },
};

const int ptn2[5][2] = {
	{ 0x01, 0x01 },
	{ 0x40, 0x40 },
	{ 0x08, 0x08 },
	{ 0x80, 0x80 },
	{ 0x00, 0x00 },
};

volatile int tsw, sw1, sw2;
int pretsw, presw1, presw2;
volatile bool sw1ps = false, sw2ps = false;

word in, spm;

void DMcw() {
	analogWrite(FIN_PIN, 80);
	digitalWrite(RIN_PIN, LOW);
}

void DMccw() {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, 80);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (++in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1ps = true;
		if (sw2 == LOW && presw2 == HIGH) sw2ps = true;

		presw1 = sw1;
		presw2 = sw2;
	}

	spm++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_4PIN);
	sw1 = presw1 = digitalRead(_USER_CON_5PIN);
	sw2 = presw2 = digitalRead(_USER_CON_3PIN);
}

void loop() {
	static int phase = 0;
	static int idx1 = 4, idx2 = 4;
	bool update = false;

	int& idx = (tsw == HIGH) ? idx1 : idx2;

	if (tsw == HIGH) {
		if (spm > 50) {
			spm = 0;
			lm.color.SM = stepm_init(phase);
			if (++phase > 3) phase = 0;
		}

		if (idx2 == 1) DMcw();
		else if (idx2 == 3) DMccw();
		else DMstop();

	} else {
		lm.color.SM = stepm_init(4);
		DMstop();
	}

	if (sw1ps) {
		sw1ps = false;
		if (--idx < 0) idx = 3;
		update = true;
	}
	if (sw2ps) {
		sw2ps = false;
		if (++idx > 3) idx = 0;
		update = true;
	}

	led_stepmotor(lm.b8);

	if (update) {
		disp(ptn1[idx1][0] | ptn2[idx2][0], ptn1[idx1][1] | ptn2[idx2][1]);
	}
}