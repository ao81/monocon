#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw;
int presw;
bool swps = false;
bool first = false;
int bzcnt = 0;
int blinkCnt = 0;

word in, tc, toneTime;

int ledidx = 0;
const int blank = 0x00;
const int segParamR[4] = { 0x01, 0x06, 0x08, 0x30 };
const int segParamL[4] = { 0x01, 0x30, 0x08, 0x06 };

const int ledParamA[3] = { B001, B101, B010 };
const int ledParamB[2] = { B111, B000 };

typedef struct {
	int phase;
	bool isOn;
} Status;
Status st;

int ph = 0, ex = 0;
void spmRotateR() {
	lm.color.SM = stepm_init(ph);
	if (++ph > 3) ph = 0;
}

void spmRotateL() {
	lm.color.SM = stepm_init(ph);
	if (--ph < 0) ph = 3;
}

void dmcw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void dmccw(int speed) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void dmstop() {
	digitalWrite(FIN_PIN, LOW);
	digitalWrite(RIN_PIN, LOW);

}

ISR(TIMER3_COMPA_vect) {
	if (++in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw = digitalRead(_USER_CON_5PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}
	tc++;

	if (toneTime > 0) {
		toneTime--;
	} else {
		noTone(BZ_PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_4PIN);
	sw = presw = digitalRead(_USER_CON_5PIN);
}

void loop() {
	if (swps) {
		swps = false;

		first = true;
		if (tsw == HIGH) st = { 1, true };
		else st = { 1, false };
	}

	if (!st.isOn) { // tsw-off
		switch (st.phase) {
		case 0: break;
		case 1:
			if (first) {
				first = false;
				ex = 0;
				tc = 40;
			}

			if (tc > 40) {
				tc = 0;
				spmRotateR();
				ex++;
			}

			if (ex >= 30) {
				st.phase = 2;
				first = true;
			}

			break;
		case 2:
			if (first) {
				first = false;
				tc = 0;
				dmcw(80);
			}

			if (tc > 2000) {
				dmstop();
				first = true;
				st.phase = 3;
			}

			break;
		case 3:
			if (first) {
				first = false;
				tc = 1000;
				ledidx = 0;
			}

			if (tc > 1000) {
				tc = 0;
				if (ledidx < 4) disp(blank, segParamR[ledidx]);
				ledidx++;
			}

			if (ledidx > 4) {
				first = true;
				disp(blank, blank);
				st.phase = 4;
			}

			break;
		case 4:
			if (first) {
				first = false;
				tc = 1000;
				ledidx = 0;
			}

			if (tc > 1000) {
				tc = 0;
				if (ledidx < 3) {
					lm.color.GBR = ledParamA[ledidx];
				}
				ledidx++;
			}

			if (ledidx > 3) {
				first = true;
				lm.color.GBR = B000;
				st.phase = 5;
			}

			break;
		case 5:
			if (first) {
				first = false;
				toneTime = 2000;
				tone(BZ_PIN, 500);
				st.phase = 0;
			}

			break;
		}
	} else {
		switch (st.phase) {
		case 0: break;
		case 1:
			if (first) {
				first = false;
				tc = 500;
				bzcnt = 0;
			}

			if (tc > 600) {
				tc = 0;
				toneTime = 100;
				if (bzcnt < 5) tone(BZ_PIN, 1500);
				bzcnt++;
			}

			if (bzcnt >= 6) {
				first = true;
				st.phase = 2;
			}

			break;

		case 2:
			if (first) {
				first = false;
				tc = 500;
				blinkCnt = 0;
			}

			if (blinkCnt <= 6 && tc > 500) {
				tc = 0;
				lm.color.GBR = ledParamB[blinkCnt % 2];
				blinkCnt++;
			}

			if (blinkCnt > 6) {
				lm.color.GBR = B000;
				first = true;
				st.phase = 3;
			}
			
			break;
		case 3:
			if (first) {
				first = false;
				tc = 1000;
				ledidx = 0;
			}

			if (tc > 1000) {
				tc = 0;
				if (ledidx < 4) disp(segParamL[ledidx], blank);
				ledidx++;
			}

			if (ledidx > 4) {
				first = true;
				disp(blank, blank);
				st.phase = 4;
			}
			
			break;
		case 4:
			if (first) {
				first = false;
				tc = 0;
				dmccw(200);
			}

			if (tc > 3000) {
				dmstop();
				first = true;
				st.phase = 5;
			}
			
			break;
		case 5:
			if (first) {
				first = false;
				ex = 0;
				tc = 40;
			}

			if (tc > 40) {
				tc = 0;
				spmRotateL();
				ex++;
			}

			if (ex >= 60) {
				st.phase = 0;
				first = true;
			}

			break;
		}
	}

	led_stepmotor(lm.b8);
}