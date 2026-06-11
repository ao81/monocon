#define USE_TIMER3
#include "mono_con.h"

int dptsw[2] = { 0x76, 0x38 };

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool phps = false;
bool r = true;

bool tgl = true;
word cd = 0;

ISR(TIMER3_COMPA_vect) {
	if (cd > 0) cd--;

	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}

	if (ph == HIGH) {
		static word acc = 0;
		acc += 30;
		if (acc >= 100) {
			acc -= 100;
			disp(dptsw[!tsw], (sw ? 0x00 : 0xff));
		} else disp(0x00, (sw ? 0x00 : 0xff));
	} else disp(dptsw[!tsw], (sw ? 0x00 : 0xff));
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = !digitalRead(pin3);
	sw = presw = !digitalRead(pin4);
	ph = preph = digitalRead(pin5);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = !digitalRead(pin3);
		sw = !digitalRead(pin4);
		ph = digitalRead(pin5);

		if (ph == HIGH && preph == LOW) phps = true;

		preph = ph;
	}

	if (phps) {
		phps = false;
		if (tsw) tone(BZ_PIN, 2000, 2000);
		else tone(BZ_PIN, 800, 2000);
	}

	if (sw == LOW) {
		if (cd <= 0) {
			cd = 500;
			lm.led(tgl ? B111 : B000).flush();
			tgl = !tgl;
		}
	} else {
		cd = 0;
		tgl = true;
		lm.led(B000).flush();
	}
}
