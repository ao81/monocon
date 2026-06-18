#define USE_TIMER3
#include "mono_con.h"

int x, y;
int ph, preph;
bool phps = false;

int buf[4] = { -1, -1, -1, -1 };
int idx = 0;

int failed = 0;
bool end = false;
bool ok = false;

int move = 0;
int cnt = 0;
bool tgl = true;

bool lock = false;

word tc, spm, t;

void resetbuf() {
	for (int i = 0; i < 4; i++) buf[i] = -1;
}

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4 + 1;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		ph = digitalRead(pin3);

		if (ph == HIGH && preph == LOW) phps = true;

		preph = ph;
	}

	tc++;
	spm++;
	t++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	ph = preph = digitalRead(pin3);

	lm.led(B001);
}

void loop() {
	int i = getdir(x, y);
	static int prei = -1;

	if (lock) {
		if (tc >= 1000) {
			tc = 0;
			if (cnt == 0) {
				lock = false;
				failed = false;
				phps = true;
			} else disp(0x00, num[--cnt]);
		}

		if (t >= 200) {
			t = 0;
			if (tgl) lm.led(B000);
			else lm.led(B001);
			tgl = !tgl;
		}

	} else {
		if (i != prei) {
			if (idx < 4) {
				if (prei == -1) {
					buf[idx++] = i;
					tone(BZ_PIN, 2000, 50);
				}
			}

			if (!end && idx == 4) {
				end = true;
				if (buf[0] == 1 && buf[1] == 2 && buf[2] == 3 && buf[3] == 4) {
					ok = true;
					lm.led(B100);
					move = 60;
					tone(BZ_PIN, 2000, 1000);
					failed = 0;
				} else {
					ok = false;
					cnt = 6;
					tc = 200;
					tone(BZ_PIN, 500, 1000);
					tgl = true;
					failed++;
				}
			}

			prei = i;
		}

		if (phps) {
			phps = false;
			if (end && ok) {
				move = -60;
			}
			resetbuf();
			idx = 0;
			lm.led(B001);
			end = ok = false;
		}

		if (cnt > 0) {
			if (tc >= 200) {
				tc = 0;
				cnt--;
				if (tgl) lm.led(B000);
				else lm.led(B001);
				tgl = !tgl;
			}
		}

		if (spm > 20) {
			spm = 0;
			if (move > 0) {
				move--;
				lm.spm(CW);
			} else if (move < 0) {
				move++;
				lm.spm(CCW);
			}
		}

		if (failed >= 3 && cnt == 0) {
			failed = 0;
			cnt = 6;
			tgl = true;
			lock = true;
			tc = 1000;
		}

		if (idx < 4) disp(num[idx + 1], 0x00);
		else disp(0x00, 0x00);
	}

	lm.flush();
}
