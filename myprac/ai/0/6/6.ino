#define USE_TIMER3
#include "mono_con.h"

const int spd = 100;

const int pass[3] = { 80, 45, 15 };
int inval[3] = { -1, -1, -1 };

int x, y;
int tsw, sw, ph;
int pretsw, preph;
bool phdps = false, phups = false;
bool input = true;

int cnt = 0;
bool ok = false;
bool tgl = true;

bool cnting = false;
bool force = false;

word tc = 0, phtc = 0, cd = 0;

int getspeed(int yy) {
	int center = 511;

	if (yy >= center) {
		int dy = yy - center;
		if (dy < 100) return -1;

		return map(dy, 100, 511, 150, 50);
	} else {
		int dy = center - yy;
		if (dy < 100) return -1;

		return -map(dy, 100, 416, 150, 50);
	}
}

void add(int n) {
	inval[2] = inval[1];
	inval[1] = inval[0];
	inval[0] = n;
}

bool cmp(int a[3], const int b[3]) {
	bool res = true;
	for (int i = 0; i < 3; i++) {
		if (a[i] != b[i]) res = false;
	}
	return res;
}

void cw() {
	analogWrite(FIN_PIN, spd);
	digitalWrite(RIN_PIN, LOW);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

typedef enum {
	IN,
	ALARM,
} status;
status st = IN;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
	}
	tc++;

	if (ph == HIGH) phtc++;
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin5);
}

void loop() {
	if (input) {
		input = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (ph == HIGH && preph == LOW) phdps = true;
		if (ph == LOW && preph == HIGH) phups = true;

		preph = ph;
	}

	if (tsw == LOW) {
		if (tsw != pretsw) {
			pretsw = tsw;
			phdps = phups = false;
		}

		switch (st) {
		case IN: {
			int speed = getspeed(y);
			if (speed == -1) {

			} else if (speed >= 0) {
				if ((int)tc >= speed) {
					tc = 0;
					lm.spm(CW);
					if (++cnt > 99) cnt = 0;
				}
			} else {
				speed = abs(speed);
				if ((int)tc >= speed) {
					tc = 0;
					lm.spm(CCW);
					if (--cnt < 0) cnt = 99;
				}
			}

			if (phdps) {
				phdps = false;
				phtc = 0;
				cnting = true;
			}

			if (cnting && phtc >= 500) {
				cnting = false;
				st = ALARM;
			}

			if (phups) {
				phups = false;
				tone(BZ_PIN, 523, 100);
				add(cnt);
			}

			break;
		}
		case ALARM:
			lm.led(B001);

			if (tc >= 1000) {
				tc = 0;
				tone(BZ_PIN, 2000, 500);
			}

			if (sw == LOW) {
				if (cd <= 0) {
					st = IN;
					cnt = 0;
				}
			} else cd = 2000;

			break;
		}

		if (st != ALARM) lm.led(B100);

		if (phups) {
			phups = false;
		}

		disp(num[cnt / 10], num[cnt % 10]);
		stop();
		force = false;

	} else {
		st = IN;

		if (!force) {
			if (tsw != pretsw) {
				pretsw = tsw;

				ok = cmp(inval, pass);
				if (ok) {
					cd = 3000;
					tgl = true;
				} else {
					tone(BZ_PIN, 500, 1000);
					cnt = 0;
					memset(inval, -1, sizeof(inval));
				}
			}

			if (ok) {
				if (cd > 0) {
					cw();
					if (ph == HIGH) {
						force = true;
						cd = 2000;
					}
					if (tc >= 250) {
						tc = 0;
						if (tgl) lm.led(B010);
						else lm.led(B000);
						tgl = !tgl;
					}
				} else {
					stop();
					lm.led(B000);
				}
			}
		} else {
			stop();
			lm.led(B001);
			if (sw == HIGH) cd = 2000;
			else if (cd <= 0) {
				force = false;
				cd = 3000;
				lm.led(B000);
			}
		}
	}

	lm.flush();
}
