#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

int stepn = 0;
int cmdn = 0;

int data[10];
bool work = false;
bool next = false;
int phase = -1;

int move = 0;
bool force = false;

word led = 0, tc = 0, cd = 0;

typedef enum {
	end,
	dc_cw,
	dc_ccw,
	st_fwd,
	st_rev,
	wait_pi,
	jump_0,
} command;
command cmd = end;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

void resetdata() {
	for (int i = 0; i < 10; i++) data[i] = 0;
}

void cw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw() {
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
		r = true;
	}

	led++;
	tc++;
	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin3);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin5);

	resetdata();
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin3);
		sw = digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	int i = getdir(x, y);

	if (tsw == LOW) {
		lm.led(B000);

		if (tsw != pretsw) {
			pretsw = tsw;
		}

		static int prei = -1;

		if (i != prei) {
			if (prei == -1) {
				if (i == 0) stepn++;
				else if (i == 2) stepn--;
				else if (i == 1) cmdn++;
				else if (i == 3) cmdn--;

				stepn = constrain(stepn, 0, 9);
				cmdn = constrain(cmdn, 0, 6);
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			data[stepn] = cmdn;
			tone(BZ_PIN, 2000, 50);
		}

		disp(num[stepn], num[cmdn]);

	} else {
		static bool tgl = true;
		static int prephase = -1;

		if (tsw != pretsw) {
			pretsw = tsw;
			phase = -1;
			tgl = true;
			work = false;
			next = false;
			force = false;
			move = 0;
		}

		if (i != -1) force = true;

		if (force) {
			work = false;
			phase = -2;
			lm.led(B001);
			disp(0x79, 0x79);
			stop();

		} else {
			if (swps) {
				swps = false;
				prephase = -1;
				phase = 0;
				work = true;
			}

			if (phase == -1) {
				lm.led(B101);
			} else {
				if (led >= 200) {
					led = 0;
					if (tgl) lm.led(B010);
					else     lm.led(B000);
					tgl = !tgl;
				}

				if (next) {
					next = false;
					if (++phase > 9) {
						work = false;
						phase = -1;
					}
				}

				if (phase >= 0) {
					cmd = (command)data[phase];

					if (phase != prephase) {
						prephase = phase;
						switch ((int)cmd) {
						case 0:
							cd = 1000;
							break;

						case 1: case 2:
							cd = 1500;
							break;

						case 3: case 4:
							move = 30;
							break;

						case 5:
							phps = false;
							break;
						}
					}
				}
			}

			disp(num[phase < 0 ? 0 : phase], num[(int)cmd]);
		}
	}

	if (work) {
		switch (cmd) {
		case end:
			if (cd > 0) {
				tone(BZ_PIN, 3000);
				lm.led(B100);
			} else {
				noTone(BZ_PIN);
				work = false;
			}

			break;
		case dc_cw:
			if (cd > 0) cw();
			else {
				stop();
				next = true;
			}

			break;
		case dc_ccw:
			if (cd > 0) ccw();
			else {
				stop();
				next = true;
			}

			break;
		case st_fwd:
			if (move != 0) {
				if (tc >= 40) {
					tc = 0;
					lm.spm(CW);
					move--;
				}
			} else next = true;

			break;
		case st_rev:
			if (move != 0) {
				if (tc >= 40) {
					tc = 0;
					lm.spm(CCW);
					move--;
				}
			} else next = true;

			break;
		case wait_pi:
			if (phps) {
				phps = false;
				next = true;
			}

			break;
		case jump_0:
			phase = 0;

			break;
		}
	} else {
		phase = -1;
	}

	lm.flush();
}
