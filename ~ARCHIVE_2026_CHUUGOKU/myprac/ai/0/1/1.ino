#define USE_TIMER3_ISR
#include "mono_con.h"

const int bz_c4 = 262;
const int speed = 100;

const int yoko = 0x49;

int tsw, sw, ph, x, y;
int pretsw, presw, preph;
bool swps = false, phpsd = false, phpsu = false;

int phase = 0;
int idx = 4;

int incar = 0;
int angle = 0;

int tgl = 1;
int cnt = 0;
bool led = false;
bool close = false;

bool sto = false;

word spm, tc;

int getidx(int xx, int yy) {
	int xidx = constrain(map(xx, 0, 1023, 0, 3), 0, 2);
	int yidx = constrain(map(yy, 0, 1023, 0, 3), 0, 2);
	return xidx + yidx * 3;
}

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

void dmcw() {
	analogWrite(FIN_PIN, speed);
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

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
		x = analogRead(A1);
		y = analogRead(A2);

		idx = getidx(x, y);

		if (sw == LOW && presw == HIGH) swps = true;
		if (tsw == HIGH && ph == HIGH && preph == LOW) phpsd = true;
		if (tsw == HIGH && ph == LOW && preph == HIGH) phpsu = true;

		presw = sw;
		preph = ph;
	}

	spm++;
	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		if (sto) sto = false;
	}

	if (swps) {
		swps = false;
		sto = true;
		tc = 500;
		tgl = 1;
	}

	if (sto) {
		lm.color.GBR = B000;
		if (tc >= 500) {
			tc = 0;
			disp(yoko, yoko);
			if (tgl) tone(BZ_PIN, 1600);
			else tone(BZ_PIN, 800);
			tgl = !tgl;
		}

	} else {
		noTone(BZ_PIN);

		if (tsw == LOW) {
			lm.color.GBR = B100;

			if (idx >= 0 && idx <= 2) {
				if (spm >= 40) {
					spm = 0;
					cw();
				}
			} else if (idx >= 6 && idx <= 8) {
				if (spm >= 40) {
					spm = 0;
					ccw();
				}
			}

			disp(0x00, 0x00);
		} else {
			if (!led) lm.color.GBR = B010;

			if (phpsd) {
				phpsd = false;

				tone(BZ_PIN, bz_c4);
				angle = 30;
			}

			if (phpsu) {
				phpsu = false;
				led = true;
				tc = 500;
				tgl = 1;
				cnt = 0;
			}

			if (led) {
				if (tc >= 500) {
					tc = 0;

					lm.color.GBR = (tgl ? B100 : B000);
					tgl = !tgl;
					if (++cnt > 6) {
						led = false;
						angle = 30;
						close = true;
					}
				}
			}

			if (angle != 0) {
				if (spm >= 40) {
					spm = 0;
					cw();
					angle--;
				}
			} else if (close) {
				close = false;
				incar++;
			} else {
				noTone(BZ_PIN);
			}

			disp(num[incar], 0x00);
		}
	}

	led_stepmotor(lm.b8);
}
