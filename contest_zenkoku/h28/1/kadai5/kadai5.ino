#define USE_TIMER3
#include "mono_con.h"

const int angles[12] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110 };

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false;
bool r = true;

int angle = 0, move = 0;
int phase = 0;

bool work = false;
bool cli = false;
bool c = false, dc = false;
bool dc2 = false;

word cd = 0, tc = 0, wait = 0;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
	if (cd > 0) cd--;
	tc++;
	if (wait > 0) wait--;
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

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (cli) {
		noInterrupts();
		if (cd > 0 && swps) {
			swps = false;
			dc2 = true;
		} else if (cd <= 0) {
			cli = false;
			c = true;
		} else if (dc2) {
			if (sw == HIGH) {
				dc2 = false;
				cli = false;
				dc = true;
				phase = 0;
			}
		}
		interrupts();
	} else {
		if (swps) {
			swps = false;
			cli = true;
			cd = 500;
		}
	}

	if (c) {
		c = false;
		work = !work;

	} else if (dc) {
		dc = false;

		work = false;
		int diff = angles[0] - angle;
		if (diff > 60) diff -= 120;
		if (diff < -60) diff += 120;
		move = diff;
	}

	if (work) {
		if (phase == 0) {
			move = (angles[3] - angle + 120) % 120;
			if (move == 0) {
				phase++;
				wait = 1000;
			}
		} else if (phase == 1) {
			if (wait <= 0) phase++;
		} else if (phase == 2) {
			int diff = (angles[9] - angle + 120) % 120;
			move = diff - 120;
			if (diff == 0) move = 0;
			if (move == 0) {
				phase++;
				wait = 1000;
			}
		} else if (phase == 3) {
			if (wait <= 0) phase = 0;
		}
	}

	if (tc >= 10) {
		tc = 0;
		if (move > 0) {
			move--;
			angle++;
			lm.spm(CW).flush();
		} else if (move < 0) {
			move++;
			angle--;
			lm.spm(CCW).flush();
		}
		angle = (angle + 120) % 120;
	}
}
