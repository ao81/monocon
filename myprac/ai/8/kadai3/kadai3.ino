// 39:43

#define USE_TIMER3
#include "mono_con.h"

const int seg[4] = { 0x77, 0x7c, 0x58, 0x5e };

int user[9];
int ans[9];
int idx = 0;

int x, y, sw;
int presw;
bool swps = false;

int level = 1;
bool first = true;

bool tgl = true;

int wait = 500;
word tc;

void reset() {
	for (int i = 0; i < 9; i++) {
		ans[i] = -1;
		user[i] = -1;
	}
}

void setans(int level) {
	for (int i = 0; i < level; i++) {
		ans[i] = random(0, 32767) % 4;
	}
}

bool cmp(int a[9], int b[9]) {
	bool res = true;
	for (int i = 0; i < 9; i++) {
		if (a[i] != b[i]) {
			res = false;
			break;
		}
	}
	return res;
}

typedef enum {
	WAIT,
	GAME,
	END,
} status;
status st = WAIT;

typedef enum {
	tehon,
	input,
} gamest;
gamest gs = tehon;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		sw = digitalRead(pin4);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	sw = presw = digitalRead(pin4);

	randomSeed(analogRead(0));
	reset();
}

void loop() {
f:

	switch (st) {
	case WAIT:
		if (swps) {
			swps = false;
			st = GAME;
			gs = tehon;
			first = true;
		}

		break;
	case GAME: {
		if (gs == tehon) {
			if (first) {
				first = false;
				setans(level);
				idx = -1;
			}

			if (tc >= wait) {
				tc = 0;
				tone(BZ_PIN, 2000, 100);

				if (idx < level - 1) {
					idx++;
				} else {
					first = true;
					disp(num[level], 0x00);
					gs = input;
					lm.led(B000);
					goto f;
				}

				if (ans[idx] == 0) lm.led(B001);
				else if (ans[idx] == 1) lm.led(B100);
				else if (ans[idx] == 2) lm.led(B010);
				else lm.led(B101);

				disp(num[level], seg[ans[idx]]);
			}

		} else {
			if (first) {
				first = false;
				idx = 0;
			}

			int i = getdir(x, y);
			static int prei = -1;

			if (i != prei) {
				if (prei == -1) {
					user[idx] = i;
					tone(BZ_PIN, 2000, 100);

					if (i == 0) lm.led(B001);
					else if (i == 1) lm.led(B100);
					else if (i == 2) lm.led(B010);
					else lm.led(B101);

					if (idx < level - 1) {
						idx++;
					} else {
						lm.led(B000);
						bool ok = cmp(ans, user);
						if (ok) {
							first = true;
							gs = tehon;
							level++;
							wait -= 50;
						} else {
							st = END;
							tone(BZ_PIN, 500, 2000);
							tgl = true;
							swps = false;
						}
					}
				}
				prei = i;
			}
		}
		break;
	}
	case END:
		if (tc >= 200) {
			tc = 0;
			if (tgl) lm.led(B001);
			else lm.led(B000);
			tgl = !tgl;
		}

		if (swps) {
			st = WAIT;
			level = 1;
			reset();
			lm.led(B000);
			wait = 500;
		}

		break;
	}

	lm.flush();
}
