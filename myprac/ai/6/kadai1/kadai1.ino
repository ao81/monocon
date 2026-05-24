#define USE_TIMER3
#include "mono_con.h"

const int led[3] = { B100, B010, B001 };
const int bz[2] = { 1500, 2000 };

int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;

bool mode = true;

int angle = 0, phase = 0;
int speed = 30;

int idx = 0;
int cnt = 0;

bool tgl = true;

word tc = 0, cd = 0;

typedef enum {
	wait,
	t1,
	t2,
	t3,
	t4,
	t5,
	end,
	intr,
} status;
status st = wait;

void cw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);

	disp(0x40, 0x40);
}

void loop() {
	if (phps) {
		phps = false;
		if (st != intr) {
			st = intr;
			stop();
			noTone(BZ_PIN);
			lm.led(B000);
			tc = 200;
			tgl = true;
			disp(0x79, 0x79);
			pretsw = tsw;
		}
	}

	switch (st) {
	case wait:
		if (swps) {
			swps = false;
			mode = (tsw == HIGH);
			if (mode) {
				speed = 15;
				angle = 60;
			} else {
				speed = 40;
				angle = 40;
			}
			st = t1;
		}

		break;
	case t1:
		if (mode) {
			if (angle > 0) {
				if (tc >= speed) {
					tc = 0;
					if (--phase < 0) phase = 3;
					lm.spm(phase);
					angle--;
				}
			} else {
				cd = 1200;
				st = t2;
			}
		} else {
			if (angle > 0) {
				if (tc >= speed) {
					tc = 0;
					if (++phase > 3) phase = 0;
					lm.spm(phase);
					angle--;
				}
			} else {
				cd = 3000;
				st = t2;
			}
		}

		break;
	case t2:
		if (mode) {
			if (cd > 0) {
				cw(180);
			} else {
				stop();
				cd = 1200;
				st = t3;
			}
		} else {
			if (cd > 0) {
				cw(80);
			} else {
				stop();
				tc = 1000;
				cnt = 1;
				st = t3;
			}
		}

		break;
	case t3:
		if (mode) {
			if (cd > 0) {
				ccw(180);
			} else {
				stop();
				idx = 0;
				tc = 400;
				st = t4;
			}
		} else {
			if (tc >= 1000) {
				tc = 0;
				if (cnt > 5) {
					cd = 2000;
					tone(BZ_PIN, 500);
					st = t4;
					break;
				}
				disp(num[0], num[cnt++]);
			}
		}

		break;
	case t4:
		if (mode) {
			if (tc >= 400) {
				tc = 0;
				if (idx > 2) {
					lm.led(B000);
					idx = 0;
					tc = 400;
					tone(BZ_PIN, 500);
					st = t5;
					break;
				}
				lm.led(led[idx++]);
			}
		} else {
			if (cd > 0) {
				lm.led(B001);
			} else {
				lm.led(B000);
				noTone(BZ_PIN);
				disp(0x40, 0x40);
				st = end;
			}
		}

		break;
	case t5:
		if (mode) {
			if (tc >= 400) {
				tc = 0;
				if (idx > 1) {
					noTone(BZ_PIN);
					disp(0x7f, 0x7f);
					st = end;
					break;
				}
				tone(BZ_PIN, bz[idx++]);
			}
		} else {
			// nothing
		}

		break;
	case intr:
		if (tc >= 200) {
			tc = 0;
			if (tgl) lm.led(B001);
			else lm.led(B000);
			tgl = !tgl;
		}

		if (tsw != pretsw) {
			disp(0x40, 0x40);
			lm.led(B000);
			st = wait;
		}

		break;
	case end:
		st = wait;
		break;
	}

	lm.flush();
}
