#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;
const int led[4] = { B000, B001, B010, B100 };
int idx = 0;
int mode = 0;

int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int phase = 0;

word tc;

void scw() {
	if (--phase < 0) phase = 3;
	lm.spm(phase);
}

void dcw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void dstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(pin_5);
		sw = digitalRead(pin_4);
		ph = digitalRead(pin_3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin_5);
	sw = presw = digitalRead(pin_4);
	ph = preph = digitalRead(pin_3);

	lm.led(led[idx]).flush();
}

void loop() {
	static bool spm = false;
	static bool dcm = false;

	if (phps) {
		phps = false;
		if (++mode > 2) mode = 0;
	}

	if (mode == 0) {
		if (swps) {
			swps = false;
			if (++idx > 3) idx = 0;
			lm.led(led[idx]);
		}
	} else if (mode == 1) {
		if (swps) {
			swps = false;
			spm = !spm;
		}
	} else if (mode == 2) {
		if (swps) {
			swps = false;
			dcm = !dcm;
		}
	}

	if (spm) {
		if (tc >= 40) {
			tc = 0;
			scw();
		}
	}

	if (dcm) dcw();
	else dstop();

	if (tsw == HIGH) disp(num[mode], 0x00);
	else disp(0x00, num[mode]);

	lm.flush();
}
