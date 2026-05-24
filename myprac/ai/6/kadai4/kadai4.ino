// 途中まで

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

word tc;

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
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A2);
	tsw = digitalRead(pin5);
	sw = digitalRead(pin4);
	ph = digitalRead(pin3);

	for (int i = 0; i < 8; i++) angles[i] = i * 15;
}

void loop() {
	if (tsw == HIGH) {
		int i = getidx(x);
		tone(BZ_PIN, pitches[i]);
		disp(0x00, num[i]);

		static int prei = -1;
		if (i != prei) {
			prei = i;
			move = getdir(angle, angles[i]);
		}

		if (tc >= 30) {
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
		}

		lm.flush();

	} else {
		noTone(BZ_PIN);
	}
}
