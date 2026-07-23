#define USE_TIMER3
#include "mono_con.h"

const int pitches[8] = { 523, 587, 659, 698, 784, 880, 988, 1047 };
int angles[8];
int input[16];

int x;
int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int angle = 0, move = 0, phase = 0;

int head = 0, phead = 0;
bool play = false;

word tc, bz, no;

void resetin() {
	for (int i = 0; i < 16; i++) input[i] = -1;
}

int getidx(int xx) {
	return constrain(map(xx, 0, 1023, 0, 8), 0, 7);
}

int getdir(int from, int to) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		x = analogRead(A2);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	tc++;
	bz++;
	if (no > 0) no--;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	for (int i = 0; i < 8; i++) angles[i] = i * 15;
	resetin();
}

void loop() {
	static int i = -1;
	static int prei = -1;

	if (tsw == HIGH) {
		i = getidx(x);

		if (no > 0) {
			noTone(BZ_PIN);
			lm.led(0b010);
		} else {
			tone(BZ_PIN, pitches[i]);
			lm.led(0b100);
		}

		if (i != prei) {
			prei = i;
			move = getdir(angle, angles[i]);
		}

		if (tc >= 10) {
			tc = 0;
			if (move > 0) {
				if (--phase < 0) phase = 3;
				lm.spm(phase);
				move--;
				angle++;
			} else if (move < 0) {
				if (++phase > 3) phase = 0;
				lm.spm(phase);
				move++;
				angle--;
			}

			angle = (angle + 120) % 120;
		}

		if (swps) {
			swps = false;

			if (head < 16) {
				input[head++] = i;
				no = 200;
			}
		}

		if (phps) {
			phps = false;
			head = 0;
			resetin();
		}

		disp(num[head / 10], num[head % 10]);

		play = false;
		phead = 0;

	} else {
		if (head == 0) {
			noTone(BZ_PIN);
			disp(0x40, 0x40);
			lm.led(0b000);
		} else {
			disp(num[phead / 10], num[phead % 10]);
			lm.led(0b001);

			if (phps) {
				phps = false;
				if (!play) {
					play = true;
					tc = 600;
					phead = 0;
				}
			}
		}

		if (play) {
			i = constrain(phead - 1, 0, 7);

			if (bz >= 600) {
				bz = 0;
				if (phead < head) {
					tone(BZ_PIN, pitches[input[phead++]], 500);
				} else {
					play = false;
				}
			}

			if (i != prei) {
				prei = i;
				move = getdir(angle, angles[i]);
			}

			if (tc >= 8) {
				tc = 0;
				if (move > 0) {
					if (--phase < 0) phase = 3;
					lm.spm(phase);
					move--;
					angle++;
				} else if (move < 0) {
					if (++phase > 3) phase = 0;
					lm.spm(phase);
					move++;
					angle--;
				}

				angle = (angle + 120) % 120;
			}

		} else noTone(BZ_PIN);

		if (swps) {
			swps = false;
			play = false;
			tone(BZ_PIN, 300, 200);
			resetin();
			head = phead = 0;
		}
	}

	lm.flush();
}
