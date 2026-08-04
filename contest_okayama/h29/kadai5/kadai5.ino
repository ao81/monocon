// 34:00

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledhands[4] = { 0x6F, 0x58, 0x73, 0x00 };
const int blank = 0x00;

int disphands[2] = { 0x00, 0x00 };
int idxhands[2] = { 3, 3 };

int tsw, sw, pretsw, presw;
bool swps = false;

int result = 0;

word in, tc;

typedef enum {
	OFF,
	Lseg,
	Rseg,
	RESULT,
} Status;
Status st;

bool first = true;

void onedisp(int n[2]) {
	static int pren[2] = { 0x00, 0x00 };
	if (n != pren) {
		memcpy(pren, n, sizeof(pren));
		disp(n[0], n[1]);
	}
}

/**
 * @return 0:あいこ　1:[0]の勝ち　2:[1]の勝ち
 */
int judge(int n[2]) {
	if (n[0] == n[1]) return 0;
	if (n[0] == 0) {
		if (n[1] == 1) return 1;
		if (n[1] == 2) return 2;
	}
	if (n[0] == 1) {
		if (n[1] == 0) return 2;
		if (n[1] == 2) return 1;
	}
	if (n[0] == 2) {
		if (n[1] == 0) return 1;
		if (n[1] == 1) return 2;
	}
}

void dm(int speed, int di) {
	if (di == 0) {
		digitalWrite(FIN_PIN, HIGH);
		digitalWrite(RIN_PIN, HIGH);
	} else {
		analogWrite(FIN_PIN, di > 0 ? speed : 0);
		analogWrite(RIN_PIN, di < 0 ? speed : 0);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);

	st = (tsw == HIGH ? Lseg : OFF);
}

void loop() {
	if (tsw == HIGH) {
		if (tsw != pretsw) {
			pretsw = tsw;
			st = Lseg;
			first = true;
		}
	} else {
		if (tsw != pretsw) {
			pretsw = tsw;
			st = OFF;
			first = true;
		}
	}

	switch (st) {
	case OFF:
		if (first) {
			first = false;
			dm(0, 0);
			idxhands[0] = idxhands[1] = 3;
			disp(blank, blank);
		}

		if (swps) swps = false;

		break;
	case Lseg:
		if (first) {
			first = false;
			tc = 200;
		}

		if (tc > 200) {
			tc = 0;
			if (++idxhands[0] > 2) idxhands[0] = 0;
			if (++idxhands[1] > 2) idxhands[1] = 0;
		}

		if (swps) {
			swps = false;
			st = Rseg;
			first = true;
		}

		break;
	case Rseg:
		if (first) {
			first = false;
			tc = 200;
		}

		if (tc > 200) {
			tc = 0;
			if (++idxhands[0] > 2) idxhands[0] = 0;
		}

		if (swps) {
			swps = false;
			st = RESULT;
			first = true;
		}

		break;
	case RESULT:
		if (first) {
			first = false;
			result = judge(idxhands);
			if (result == 1) dm(80, -1);
			else if (result == 2) dm(80, 1);
		}

		break;
	}

	if (st != OFF) {
		disphands[0] = ledhands[idxhands[0]];
		disphands[1] = ledhands[idxhands[1]];
		onedisp(disphands);
	}
}