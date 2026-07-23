#define USE_TIMER3
#include "mono_con.h"

const int seg1[4] = { 0x01, 0x06, 0x08, 0x30 };
const int seg2[4] = { 0x01, 0x30, 0x08, 0x06 };
const int led[3] = { B001, B101, B010 };

int x, y, tsw, sw, ph;
int p, presw, preph;
bool swps = false, phps = false;
bool r = true;

int cnt = 0;
int phase = 0;
bool tgl = true;
int idx = 0;

int move = 0;

word tc = 0, cd = 0;

void cw(int spd) {
	analogWrite(FIN_PIN, spd);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int spd) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, spd);
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
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = p = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin3);
		sw = digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	if (phase == 0) {
		if (swps) {
			swps = false;
			p = tsw;
			phase = 1;
			if (p) {
				cnt = 5;
				tc = 200;
			} else {
				move = 30;
			}
		}
	} else if (phase == 1) {
		if (p) {
			if (tc >= 200) {
				tc = 0;
				if (cnt > 0) {
					cnt--;
					tone(BZ_PIN, 2000, 100);
				} else {
					phase = 2;
					cd = 3000;
					tc = 250;
					tgl = true;
				}
			}
		} else {
			if (tc >= 30) {
				tc = 0;
				if (move > 0) {
					move--;
					lm.spm(CW);
				} else {
					phase = 2;
					cd = 2000;
				}
			}
		}
	} else if (phase == 2) {
		if (p) {
			if (cd > 0) {
				if (tc >= 250) {
					tc = 0;
					if (tgl) lm.led(B111);
					else lm.led(B000);
					tgl = !tgl;
				}
			} else {
				phase = 3;
				cd = 0;
				idx = 0;
			}
		} else {
			if (cd > 0) {
				cw(90);
			} else {
				stop();
				phase = 3;
				idx = 0;
				cd = 0;
			}
		}
	} else if (phase == 3) {
		if (p) {
			if (cd <= 0) {
				cd = 1000;
				if (idx < 4) {
					disp(seg2[idx++], 0x00);
				} else {
					phase = 4;
					disp(0x00, 0x00);
					cd = 3000;
				}
			}
		} else {
			if (cd <= 0) {
				cd = 1000;
				if (idx < 4) {
					disp(seg1[idx++], 0x00);
				} else {
					phase = 4;
					disp(0x00, 0x00);
					idx = 0;
					cd = 0;
				}
			}
		}
	} else if (phase == 4) {
		if (p) {
			if (cd > 0) {
				ccw(200);
			} else {
				stop();
				phase = 5;
				move = -60;
			}
		} else {
			if (cd <= 0) {
				cd = 1000;
				if (idx < 3) {
					lm.led(led[idx++]);
				} else {
					phase = 5;
					lm.led(B000);
					cd = 2000;
					tone(BZ_PIN, 800);
				}
			}
		}
	} else if (phase == 5) {
		if (p) {
			if (tc >= 30) {
				tc = 0;
				if (move < 0) {
					move++;
					lm.spm(CCW);
				} else {
					phase = 0;
				}
			}
		} else {
			if (cd <= 0) {
				noTone(BZ_PIN);
				phase = 0;
			}
		}
	}

	lm.flush();
}
