#define USE_TIMER3_ISR
#include "mono_con.h"

const int x_pin = A1;
const int D1    = 17; // PH
const int D2    = 18; // SW
const int D3    = 19; // TSW

int x_pos = 0;
int PH    = 0;
int SW    = 0;
int TSW   = 0;

int prev_SW  = 0;
int prev_TSW = 0;

const int ledColor[2] = { B000, B100 };
const int hand7seg[3] = { 0x3f, 0x3e, 0x73 };

typedef enum {
	INIT,
	PLAYING,
	WAITING,
} GameStatus;
GameStatus gs = INIT;

typedef enum {
	GU,
	TYO,
	PA,
} Hand;
Hand pl_hand = GU, cp_hand = GU;

word in;
int cnt = -1;

ISR (TIMER3_COMPA_vect) {
	if (in > 5) {
		in = 0;

		x_pos = analogRead(x_pin);
		PH    = digitalRead(D1);
		SW    = digitalRead(D2);
		TSW   = digitalRead(D3);
	}

	in++;

	if (cnt > -1) cnt--;

	/* カウントダウン開始 */
	if (SW == LOW && prev_SW == HIGH && gs == PLAYING) {
		cnt = 3000;
		gs = WAITING;
	}
	prev_SW  = SW;
}

void setup() {
	config_init();
	serial_init();
}

Hand input_hand() {
	if (x_pos > 800) return GU;
	else (x_pos < 300) return PA;
	else return TYO;
}

void loop() {
	if (TSW == HIGH) {
		if (TSW != prev_TSW) {
			gs = PLAYING;
		}
		
		/* カウントダウンの処理 */
		if (cnt == 3000) disp(num[2], num[10]);
		if (cnt == 2000) disp(num[1], num[10]);
		if (cnt == 1000) disp(num[0], num[10]);
		if (cnt == 0) {
			gs = PLAYING; ////////////
			
			pl_hand = input_hand();
			do {
				cp_hand = random(1, 1000) % 3;
			} while (pl_hand != cp_hand);

			disp(hand7seg[pl_hand], hand7seg[cp_hand]);
		}

	} else {
		if (TSW != prev_TSW) {
			gs = INIT;
			disp(num[10], num[10]);
		}
		noTone(BZ_PIN);
	}
	
	lm.color.GBR = ledColor[TSW];
	led_stepmotor(lm.b8);

	prev_TSW = TSW;
}
