#define USE_TIMER3
#include "mono_con.h"

const int dspd[3] = { 200, 150, 90 };
const int sspd[3] = { 60, 30, 10 };

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

int ondo = 20;
bool down = false;
word tc = 0, cd = 0, sp = 0;

void cw(int spd) {
	analogWrite(FIN_PIN, spd);
	digitalWrite(RIN_PIN, LOW);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	tc++;
	sp++;
	if (cd > 0) cd--;

	static word t = 500;
	static bool tgl = true;

	if (ondo == 15) {
		static word acc = 0;
		acc += 10;
		if (acc >= 100) {
			acc -= 100;
			disp(num[ondo / 10], num[ondo % 10]);
		} else disp(0x00, 0x00);
		t = 500;
		tgl = true;
	} else if (ondo == 30) {
		if (++t > 500) {
			t = 0;
			if (tgl) disp(num[ondo / 10], num[ondo % 10]);
			else disp(0x00, 0x00);
			tgl = !tgl;
		}
	} else {
		disp(num[ondo / 10], num[ondo % 10]);
		t = 500;
		tgl = true;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;
		x = analogRead(A2);
		y = analogRead(A1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	auto w = [](bool ts) {
		if (ts) {
			if (ondo < 30) ondo++;
		} else {
			if (ondo > 15) ondo--;
		}
	};

	if (swps) {
		swps = false;
		cd = 1000;
		down = true;
		w(tsw);
	}

	if (sw == HIGH) down = false;

	if (cd <= 0 && down) {
		if (tc >= 500) {
			tc = 0;
			w(tsw);
		}
	} else tc = 500;

	static int i = 0;
	if (ondo <= 20) i = 0;
	else if (ondo <= 25) i = 1;
	else i = 2;

	cw(dspd[i]);
	if (sp >= sspd[i]) {
		sp = 0;
		lm.spm(CW).flush();
	}
}
