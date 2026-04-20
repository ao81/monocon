// 40:50 途中まで

#define USE_TIMER3_ISR
#include "mono_con.h"

const int japan[9] = { 0x00, 0x00, 0x1E, 0x77, 0x73, 0x77, 0x54, 0x00, 0x00 };
const int yahoo[9] = { 0x00, 0x00, 0x6E, 0x77, 0x76, 0x5C, 0x5C, 0x00, 0x00 };
const int usa[7] = { 0x00, 0x00, 0x3E, 0x6D, 0x77, 0x00, 0x00 };
const int apple[9] = { 0x00, 0x00, 0x77, 0x73, 0x73, 0x38, 0x79, 0x00, 0x00 };
const int china[9] = { 0x00, 0x00, 0x39, 0x76, 0x06, 0x54, 0x77, 0x00, 0x00 };
const int google[10] = { 0x00, 0x00, 0x6F, 0x5C, 0x5C, 0x6F, 0x54, 0x79, 0x00, 0x00 };

const int* patterns[6] = { japan, yahoo, usa, apple, china, google };
const int lengths[6] = { 9, 9, 7, 9, 9, 10 };

int index = 0;
int len = 0;

int country = 1;

int tsw, ph, sw;
int preph, presw;
bool phps = false, swps = false;

word in, tc;

enum class status {
	wait,
	japan,
	yahoo,
	usa,
	apple,
	china,
	google
};
status st = status::wait;

void onedisp(int n, int m) {
	static int pren = 0x00, prem = 0x00;
	if (pren != n || prem != m) {
		pren = n, prem = m;
		disp(n, m);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		ph = digitalRead(_USER_CON_3PIN);
		sw = digitalRead(_USER_CON_4PIN);

		if (st == status::wait && sw == LOW && presw == HIGH) swps = true;
		if (st == status::wait && ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (st != status::wait) tc++;
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);

	disp(0x00, num[1]);
}

void loop() {
	switch (st) {
	case status::wait:
		if (swps) {
			swps = false;
			if (++country > 3) country = 1;
		}

		if (phps) {
			phps = false;

			if (country == 1) {
				if (tsw == HIGH) st = status::yahoo;
				else st = status::japan;
			} else if (country == 2) {
				if (tsw == HIGH) st = status::apple;
				else st = status::usa;
			} else {
				if (tsw == HIGH) st = status::google;
				else st = status::china;
			}
		}

		break;
	case status::japan:


		break;
	case status::yahoo:


		break;
	case status::usa:


		break;
	case status::apple:


		break;
	case status::china:


		break;
	case status::google:


		break;
	}

	if (st == status::wait) onedisp(0x00, num[country]);
	// else onedisp()
}