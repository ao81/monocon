#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false;
bool phdps = false, phups = false;
bool r = true;

int time = 0;
int cnt[3] = { 0 };

int idx = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}

	if (ph == HIGH) {
		time++;
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

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == HIGH && preph == LOW) phdps = true;
		if (ph == LOW && preph == HIGH) phups = true;

		presw = sw;
		preph = ph;
	}

	if (phdps) {
		phdps = false;
		time = 0;
		idx = 0;
	}

	if (phups) {
		phups = false;
		if (time < 300) {
			if (cnt[0] < 9) cnt[0]++;
			disp(num[1], num[cnt[0]]);
			lm.led(B010);
			tone(BZ_PIN, 2000, 1000);
		} else if (time <= 800) {
			if (cnt[1] < 9) cnt[1]++;
			disp(num[2], num[cnt[1]]);
			lm.led(B100);
			tone(BZ_PIN, 1600, 1000);
		} else {
			if (cnt[2] < 9) cnt[2]++;
			disp(num[3], num[cnt[2]]);
			lm.led(B001);
			tone(BZ_PIN, 1200, 1000);
		}
		lm.flush();
	}

	if (swps) {
		swps = false;
		if (idx == 0) {
			disp(num[1], num[cnt[0]]);
			lm.led(B010);
		} else if (idx == 1) {
			disp(num[2], num[cnt[1]]);
			lm.led(B100);
		} else {
			disp(num[3], num[cnt[2]]);
			lm.led(B001);
		}
		if (++idx > 2) idx = 0;
		lm.flush();
	}

	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			disp(0x00, 0x00);
			lm.led(B000).flush();
			memset(cnt, 0, sizeof(cnt));
		}
	} else if (tsw != pretsw) pretsw = tsw;
}
