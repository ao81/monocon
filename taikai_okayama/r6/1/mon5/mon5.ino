#define USE_TIMER3_ISR
#include "mono_con.h"

int TSW = LOW, SW = HIGH;
int preTSW = LOW, preSW = HIGH;

const int seikou[2] = { 0x7f, 0x00 };
const int sippai[2] = { 0x49, 0x00 };

int flushCount = 0;

const int speed = 30;
int phase = 0, rotate = 0;

bool pushed = false;
bool success = false;

typedef enum {
	INIT,
	COUNTDOWN,
	ROTATE,
	FAIL_WAIT,   // 失敗待機状態
	FAIL_ROTATE, // 失敗振り下ろし状態
	RESULT,
	END,
} GameStates;
GameStates gs = INIT, pregs = INIT;

bool disp2 = false, disp1 = false, disp0 = false;

word in = 0, spm = 0;
word waitCount = 0, flush = 0;
int cd = 0;

int tgl = 0; // RESULT時の点滅

ISR(TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;
		TSW = digitalRead(18);
		SW = digitalRead(19);
	}
	in++;

	if (gs == COUNTDOWN) {
		if (cd > -1) cd--;
	} else {
		cd = 3000;
	}

	if (gs == ROTATE || gs == FAIL_ROTATE) spm++;
	else spm = 0;

	if (gs == FAIL_WAIT) waitCount++;

	if (gs == RESULT) flush++;
}

void setup() {
	config_init();
	serial_init();
	disp(num[10], num[10]);
}

void loop() {
	if (TSW == HIGH) {
		if (preTSW == LOW) gs = COUNTDOWN;
	} else {
		gs = INIT;
	}

	GameStates tmpgs = gs;

	switch (gs) {
	case INIT:
		if (gs != pregs) {
			disp(num[10], num[10]);
			rotate = 0;
			phase = 0;
			pushed = false;
			success = false;
		}
		break;

	case COUNTDOWN:
		if (gs != pregs) {
			disp(num[3], num[10]);
			disp2 = disp1 = disp0 = false;
			pushed = false;
			rotate = 0;
			phase = 0;
		}
		if (cd <= 2000 && !disp2) { disp(num[2], num[10]); disp2 = true; }
		if (cd <= 1000 && !disp1) { disp(num[1], num[10]); disp1 = true; }
		if (cd <= 0 && !disp0) { disp(num[0], num[10]); disp0 = true; gs = ROTATE; }
		break;

	case ROTATE:
		if (spm > speed && rotate < 30) {
			spm = 0;
			rotate++;
			lm.color.SM = stepm_init(phase);
			led_stepmotor(lm.b8);
			if (++phase > 3) phase = 0;
		}

		if (!pushed && SW == LOW && preSW == HIGH) {
			pushed = true;
			success = (rotate >= 12 && rotate <= 18);
			lm.color.SM = 0x00;
			led_stepmotor(lm.b8);
			gs = success ? RESULT : FAIL_WAIT;
		}

		if (rotate >= 30) {
			success = false;
			lm.color.SM = 0x00;
			led_stepmotor(lm.b8);
			gs = FAIL_WAIT;
		}
		break;

	case FAIL_WAIT:
		if (gs != pregs) {
			waitCount = 0;
		}
		if (waitCount >= 1000) {
			gs = FAIL_ROTATE;
		}
		break;

	case FAIL_ROTATE:
		if (spm > speed && rotate < 30) {
			spm = 0;
			rotate++;
			lm.color.SM = stepm_init(phase);
			led_stepmotor(lm.b8);
			if (++phase > 3) phase = 0;
		}
		if (rotate >= 30) {
			lm.color.SM = 0x00;
			led_stepmotor(lm.b8);
			gs = RESULT;
		}
		break;

	case RESULT:
		if (gs != pregs) {
			flush = 0;
			tgl = 0;
			flushCount = 0;

			if (success) disp(seikou[tgl], seikou[tgl]);
			else disp(sippai[tgl], sippai[tgl]);
		}

		if (flush > 500) {
			flush = 0;
			flushCount++;
			tgl = !tgl;

			if (success) disp(seikou[tgl], seikou[tgl]);
			else disp(sippai[tgl], sippai[tgl]);
		}

		if (flushCount >= 5) gs = END;
		break;

	case END:
		break;
	}

	preTSW = TSW;
	preSW = SW;
	pregs = tmpgs;
}