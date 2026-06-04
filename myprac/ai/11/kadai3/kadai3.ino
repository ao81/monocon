#define USE_TIMER3
#include "mono_con.h"

const int seghand[4] = { 0x77, 0x7c, 0x58, 0x00 };

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool input = true;

bool first = true;
int cnt = 0;
word tc;

typedef enum {
	WAIT,
	SELECT,
	RES,
} status;
status st = WAIT;

typedef enum {
	GU,
	TYO,
	PA,
	NONE,
} hand;
hand pl, cp;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

int winner(hand a, hand b) {
	if (a == b) return 0;
	if (a == GU) {
		if (b == TYO) return 1;
		else return 2;
	} else if (a == TYO) {
		if (b == GU) return 2;
		else return 1;
	} else if (a == PA) {
		if (b == GU) return 2;
		else return 1;
	}
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();
	randomSeed(analogRead(A15));

	sw = presw = digitalRead(pin4);
}

void loop() {
	if (input) {
		input = false;
		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	switch (st) {
	case WAIT:
		if (swps) {
			swps = false;
			st = SELECT;
			pl = NONE;
			cp = (hand)random(0, 3);
			first = true;
		}

		disp(0x00, 0x00);

		break;
	case SELECT: {
		int i = getdir(x, y);
		static int prei = -2;
		if (i != prei) {
			if (prei == -1) {
				if (i == 0) pl = GU;
				else if (i == 1) pl = TYO;
				else if (i == 2) pl = PA;
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			if (pl != NONE) {
				st = RES;
			}
		}

		disp(0x00, seghand[(int)pl]);

		break;
	}
	case RES: {
		int res;
		if (first) {
			first = false;
			res = winner(pl, cp);

			if (res == 0) {
				tone(BZ_PIN, 1000, 1000);
				lm.led(B101);
			} else if (res == 1) {
				cnt = 2;
				lm.led(B100);
			} else {
				tone(BZ_PIN, 800, 1000);
				lm.led(B001);
			}
			lm.flush();
		}

		if (cnt > 0) {
			if (tc >= 500) {
				tc = 0;
				tone(BZ_PIN, 2000, 250);
				cnt--;
			}
		}

		if (swps) {
			swps = false;
			st = WAIT;
			noTone(BZ_PIN);
			lm.led(B000).flush();
		}

		disp(seghand[(int)cp], seghand[(int)pl]);

		break;
	}
	}
}
