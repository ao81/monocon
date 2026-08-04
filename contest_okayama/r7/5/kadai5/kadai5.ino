#define USE_TIMER3_ISR
#include "mono_con.h"

const int seg[3] = { 0x3f, 0x3e, 0x73 };
const int step[3] = { 3, 6, 6 };

const int bz[11] = { 254, 262, 277, 293, 311, 329, 349, 391, 440, 493, 523 };
enum { B3b, C4b, Df4b, D4b, Ef4b, E4b, F4b, G4b, A4b, B4b, C5b };

const int bzg[3] = { F4b, B3b, C4b };
const int bzt[6] = { C4b, E4b, G4b, C5b, B3b, C4b };
const int bzp[6] = { E4b, D4b, C4b, A4b, B3b, C4b };

const int *bz_step[3] = { bzg, bzt, bzp };

const int plff[6][2] = {
	{ C4b, 400 },
	{ E4b, 200 },
	{ G4b, 200 },
	{ C5b, 800 },
	{ B4b, 800 },
	{ C5b, 400 },
};

const int cpff[4][2] = {
	{ C4b,  50 },
	{ Df4b, 50 },
	{ D4b,  50 },
	{ Ef4b, 50 },
};

int idx = 0;

int tsw, sw, x;
int pretsw, presw;
bool swps = false;

int phase = 0;
int cnt = 0;

int jd = 0;

int total = 0;

word in, tc, cd, bzz;

typedef enum {
	INIT,
	WAIT,
	CD,
	RES,
	FANFARE,
	END,
} Status;
Status st;

typedef enum {
	GU,
	TYO,
	PA,
} Hand;
Hand plhand, cphand;

int gethand(int n) {
	return constrain(map(n, 0, 1023, 2, -1), 0, 2);
}

// winner: 0->a 1->b
int judge(Hand a, Hand b) {
	if (a == b) return 2;
	if (a == GU) {
		if (b == TYO) return 0;
		if (b == PA) return 1;
	} else if (a == TYO) {
		if (b == GU) return 1;
		if (b == PA) return 0;
	} else {
		if (b == GU) return 0;
		if (b == TYO) return 1;
	}
}

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		x = analogRead(A1);

		if (st == WAIT && sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;

	if (cd > 0) cd--;

	if (bzz > 0) {
		bzz--;
		if (bzz == 0) noTone(BZ_PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	x = analogRead(A1);

	pretsw = !tsw;

	randomSeed(analogRead(0));
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			st = WAIT;
			lm.color.GBR = B100;
		}
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			st = INIT;
			total = 0;
			lm.color.GBR = B000;
		}
	}

	switch (st) {
	case INIT:
		disp(0x00, 0x00);
		noTone(BZ_PIN);

		break;
	case WAIT:
		if (abs(total) >= 14) {
			idx = 1;
			tc = 0;
			st = FANFARE;
			tone(BZ_PIN, bz[(total < 0 ? plff[0][0] : cpff[0][0])]);
		}

		if (swps) {
			swps = false;
			st = CD;
			cd = 3000;
		}

		break;
	case CD: {
		int n = (cd + 999) / 1000;
		disp(num[(cd > 0 ? n - 1 : 0)], 0x00);

		if (cd == 0) {
			plhand = (Hand)gethand(x);
			// cphand = (Hand)random(0, 3);
			cphand = PA;
			st = RES;
			jd = judge(plhand, cphand);
			tc = 400;
			cnt = 0;
		}

		break;
	}
	case RES:
		if (jd == 0) {
			if (tc >= 400) {
				tc = 0;
				ccw();
				tone(BZ_PIN, bz[bz_step[plhand][cnt]]);
				bzz = 400;
				total--;
				if (++cnt >= step[plhand]) {
					st = WAIT;
				}
			}

			disp(seg[(int)plhand] | 0x80, seg[(int)cphand]);
		} else if (jd == 1) {
			if (tc >= 400) {
				tc = 0;
				cw();
				tone(BZ_PIN, bz[bz_step[cphand][cnt]]);
				bzz = 400;
				total++;
				if (++cnt >= step[cphand]) {
					st = WAIT;
				}
			}

			disp(seg[(int)plhand], seg[(int)cphand] | 0x80);
		} else {
			disp(seg[(int)plhand], seg[(int)cphand]);
			st = WAIT;
		}

		break;
	case FANFARE:
		if (total < 0) {
			if (tc >= plff[idx][1]) {
				tc = 0;
				tone(BZ_PIN, bz[plff[idx][0]]);
				bzz = plff[idx][1];
				idx++;
				if (idx > 6) st = END;
			}
		} else {
			if (tc >= cpff[idx % 4][1]) {
				tc = 0;
				tone(BZ_PIN, bz[cpff[idx % 4][0]]);
				bzz = cpff[idx % 4][1];
				idx++;
				if (idx > 4*4) st = END;
			}
		}
		break;
	case END:
		break;
	}

	if (swps) swps = false;

	led_stepmotor(lm.b8);
}


