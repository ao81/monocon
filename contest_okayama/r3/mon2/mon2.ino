// RCサーボの代わりにフルカラーLEDを用いる

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw1, sw2;
int presw1, presw2;
bool sw1ps = false, sw2ps = false;

bool spm = false, dcm = false, led = false;

int phase = 0;

word in, sm;

void DMcw() {
	analogWrite(FIN_PIN, 80);
	digitalWrite(RIN_PIN, LOW);
}

void DMstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1ps = true;
		if (sw2 == LOW && presw2 == HIGH) sw2ps = true;

		presw1 = sw1;
		presw2 = sw2;
	}

	in++;

	if (spm) sm++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_4PIN);
	sw1 = digitalRead(_USER_CON_5PIN);
	sw2 = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (sw1ps) {
		sw1ps = false;
		spm = !spm;
	}

	if (sw2ps) {
		sw2ps = false;
		dcm = !dcm;
	}

	if (spm) {
		if (sm > 50) {
			sm = 0;
			lm.color.SM = stepm_init(phase);
			if (++phase > 3) phase = 0;
		}
	} else {
		sm = 50;
	}

	if (dcm) {
		DMcw();
	} else {
		DMstop();
	}


	if (tsw == HIGH) {
		lm.color.GBR = B100;
	} else {
		lm.color.GBR = B000;
	}

	led_stepmotor(lm.b8);
}