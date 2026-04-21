// 1:19:58
// プログラム合計時間 01:51:18
// 全体合計時間 02:31:40

#define USE_TIMER3_ISR
#include "mono_con.h"

const int lo = 0 + 200;
const int hi = 1023 - 200;

const int segHand[3] = { 0x3F, 0x3E, 0x73 };
const int handLen[3] = { 3, 6, 6 };

const int pitches[11] = { 254, 262, 277, 293, 311, 329, 349, 391, 440, 493, 523 };

const int guriko[3] = { 6, 0, 1 };
const int tyokoreito[6] = { 1, 5, 7, 10, 0, 1 };
const int painappuru[6] = { 5, 3, 1, 8, 0, 1 };

// { pitch_idx, ms }
const int pl_fanfare[6][2] = {
	{ 1, 400 },
	{ 5, 200 },
	{ 7, 200 },
	{ 10, 800 },
	{ 9, 800 },
	{ 10, 400 },
};

const int cp_fanfare[4][2] = {
	{ 1, 50 },
	{ 4, 50 },
	{ 3, 50 },
	{ 4, 50 },
};

int ff_idx = 0, ff_cnt = 0;

int tsw, sw, ph, xpos;

int cdcnt = 2, winres;
int phase = 0, stepcnt = 0;

int kaidan = 0;

bool first = true;

word in, cd, spm, bz, se;

typedef enum {
	GU,
	TYO,
	PA,
} Hand;
Hand pl_hand, cp_hand;
Hand win_hand;

typedef enum {
	INIT,
	WAIT,
	COUNTDOWN,
	RESULT,
	FANFARE,
	END,
} Status;
Status st = INIT;

void cw() {
	lm.color.SM = stepm_init(phase);
	if (++phase > 3) phase = 0;
}

void ccw() {
	lm.color.SM = stepm_init(phase);
	if (--phase < 0) phase = 3;
}

Hand input_hand() {
	int x = xpos;
	if (x > hi) return GU;
	else if (x < lo) return PA;
	else return TYO;
}

// 0:あいこ　1:pの勝ち　2:cの勝ち
int winner(Hand p, Hand c) {
	if (p == c) return 0;
	if (p == GU) {
		if (c == TYO) return 1;
		else return 2;
	}
	if (p == TYO) {
		if (c == GU) return 2;
		else return 1;
	}
	if (p == PA) {
		if (c == GU) return 1;
		else return 2;
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);
		xpos = analogRead(A1);
	}

	cd++;
	spm++;
	bz++;

	if (se > 0) se--;
	else {
		if (st != FANFARE) noTone(BZ_PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = digitalRead(_USER_CON_4PIN);
	ph = digitalRead(_USER_CON_3PIN);
	xpos = analogRead(A1);

	randomSeed(millis);
}

void loop() {
	if (tsw == LOW) {
		st = INIT;
		first = true;
		lm.color.GBR = B000;
	} else if (st == INIT) {
		st = WAIT;
		first = true;
		lm.color.GBR = B100;
	}

	switch (st) {
	case INIT:
		if (first) {
			first = false;
			disp(0x00, 0x00);
			kaidan = 0;
		}

		break;
	case WAIT:
		if (first) { first = false; }

		if (sw == LOW) {
			st = COUNTDOWN;
			first = true;
		}

		break;
	case COUNTDOWN:
		if (first) {
			first = false;
			cd = 1000;
			cdcnt = 2;
		}

		if (cdcnt >= -1) {
			if (cd >= 1000) {
				cd = 0;
				disp(num[cdcnt], 0x00);
				cdcnt--;
			}
		} else {
			st = RESULT;
			first = true;
		}

		break;
	case RESULT:
		if (first) {
			first = false;

			pl_hand = input_hand();
			cp_hand = (Hand)random(3);

			winres = winner(pl_hand, cp_hand);
			if (winres == 1) {
				disp(segHand[pl_hand] | 0x80, segHand[cp_hand]);
				win_hand = pl_hand;
			} else if (winres == 2) {
				disp(segHand[pl_hand], segHand[cp_hand] | 0x80);
				win_hand = cp_hand;
			} else {
				disp(segHand[pl_hand], segHand[cp_hand]);
				first = true;
				st = WAIT;
				break;
			}

			spm = 400;
			stepcnt = 0;
		}

		if (stepcnt < handLen[win_hand]) {
			if (spm >= 400) {
				spm = 0;
				if (winres == 1) {
					ccw();
					kaidan--;
				} else {
					cw();
					kaidan++;
				}
				if (win_hand == GU) tone(BZ_PIN, pitches[guriko[stepcnt]]);
				else if (win_hand == TYO) tone(BZ_PIN, pitches[tyokoreito[stepcnt]]);
				else tone(BZ_PIN, pitches[painappuru[stepcnt]]);
				se = 400;
				stepcnt++;
			}
		} else {
			first = true;
			if (abs(kaidan) >= 14) {
				st = FANFARE;
			} else {
				st = WAIT;
			}
		}

		break;
	case FANFARE:
		if (first) {
			first = false;
			ff_idx = 0;
			ff_cnt = 0;
			bz = 0;
			if (winres == 1) {
				tone(BZ_PIN, pitches[pl_fanfare[0][0]]);
			} else {
				tone(BZ_PIN, pitches[cp_fanfare[0][0]]);
			}
		}

		if (winres == 1) {
			if (ff_idx < 6) {
				if (bz >= pl_fanfare[ff_idx][1]) {
					bz = 0;
					ff_idx++;
					if (ff_idx < 6) {
						tone(BZ_PIN, pitches[pl_fanfare[ff_idx][0]]);
					}
				}
			} else {
				noTone(BZ_PIN);
				first = true;
				st = END;
			}

		} else {
			if (ff_cnt < 16) {
				if (bz >= cp_fanfare[ff_idx][1]) {
					bz = 0;
					ff_cnt++;
					if (++ff_idx > 3) ff_idx = 0;
					if (ff_cnt < 17) {
						tone(BZ_PIN, pitches[cp_fanfare[ff_idx][0]]);
					}
				}
			} else {
				noTone(BZ_PIN);
				first = true;
				st = END;
			}
		}

		break;
	case END: break;
	}

	led_stepmotor(lm.b8);
}