#define USE_TIMER3_ISR
#include "mono_con.h"

typedef enum {
	INIT,
	WAIT,
	COUNTDOWN,
	JUDGE,
	STEP,
	FANFARE,
	END,
} GameStates;
GameStates gs = INIT, pregs = INIT;

typedef enum {
	GU,
	TYO,
	PA,
} Hand;
Hand pl_hand = GU, cp_hand = GU;

const int hand7seg[3] = { 0x3f, 0x3e, 0x73 };

int TSW = LOW, SW = HIGH;
int preTSW = LOW, preSW = HIGH;

bool cd2, cd1, cd0;

word in = 0;
int cd = -1;

int result = 0;

Hand input_hand() {
	int x_pos = analogRead(A1);
	if (x_pos > 700) return GU;
	else if (x_pos < 300) return PA;
	else return TYO;
}

/**
 * @return 0:あいこ 1:pの勝ち 2:cの勝ち
 */
int judge_winner(Hand p, Hand c) {
	if (p == c) return 0;
	if (p == GU) {
		if (c == TYO) return 1;
		if (c == PA) return 2;
	}
	if (p == TYO) {
		if (c == GU) return 2;
		if (c == PA) return 1;
	}
	if (p == PA) {
		if (c == GU) return 1;
		if (c == TYO) return 2;
	}
	return 0;
}

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		TSW = digitalRead(19);
		SW = digitalRead(18);
	}

	in++;

	if (cd > -1) cd--;
}

void setup() {
	config_init();
	serial_init();

	randomSeed(millis());
}

void loop() {
	if (TSW == LOW) {
		gs = INIT;
	} else {
		if (TSW != preTSW) gs = WAIT;
	}

	GameStates gs_ = gs;

	switch (gs) {
	case INIT:
		lm.color.GBR = B000;
		if (gs != pregs) disp(num[10], num[10]);

		break;
	case WAIT:
		if (SW == LOW && preSW == HIGH) gs = COUNTDOWN;
		lm.color.GBR = B100;
		if (gs != pregs) disp(num[10], num[10]);

		break;
	case COUNTDOWN:
		if (gs != pregs) {
			cd = 3000;
			disp(num[2], num[10]);
			cd2 = cd1 = cd0 = false;
		}
		if (!cd1 && cd <= 2000) { disp(num[1], num[10]); cd1 = true; }
		if (!cd0 && cd <= 1000) { disp(num[0], num[10]); cd0 = true; }
		if (cd <= 0) gs = WAIT;

		break;
	case JUDGE:
		// pl_hand = input_hand();
		// cp_hand = (Hand)random(3);

		result = judge_winner(pl_hand, cp_hand);
		if (result == 1) { // plの勝ち
			disp(hand7seg[(int)pl_hand] | 0x80, hand7seg[(int)cp_hand]);
		} else if (result == 2) { // cpの勝ち
			disp(hand7seg[(int)pl_hand], hand7seg[(int)cp_hand] | 0x80);
		} else { // あいこ
			disp(hand7seg[(int)pl_hand], hand7seg[(int)cp_hand]);
		}

		gs = WAIT;

		break;
	case STEP:

		break;
	case FANFARE:

		break;
	case END:

		break;
	}

	led_stepmotor(lm.b8);

	preTSW = TSW;
	preSW = SW;
	pregs = gs_;
}