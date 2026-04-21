// 71:51
// 合計時間 02:06:09（課題4無し）

#define USE_TIMER3_ISR
#include "mono_con.h"

const int japan[9] = { 0x00, 0x00, 0x1E, 0x77, 0x73, 0x77, 0x54, 0x00, 0x00 };
const int yahoo[9] = { 0x00, 0x00, 0x6E, 0x77, 0x76, 0x5C, 0x5C, 0x00, 0x00 };
const int usa[7] = { 0x00, 0x00, 0x3E, 0x6D, 0x77, 0x00, 0x00 };
const int apple[9] = { 0x00, 0x00, 0x77, 0x73, 0x73, 0x38, 0x79, 0x00, 0x00 };
const int china[9] = { 0x00, 0x00, 0x39, 0x76, 0x06, 0x54, 0x77, 0x00, 0x00 };
const int google[10] = { 0x00, 0x00, 0x6F, 0x5C, 0x5C, 0x6F, 0x38, 0x79, 0x00, 0x00 };

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

int statusToPatternIndex(status s) {
	switch (s) {
	case status::japan:  return 0;
	case status::yahoo:  return 1;
	case status::usa:    return 2;
	case status::apple:  return 3;
	case status::china:  return 4;
	case status::google: return 5;
	default:             return 0;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);

	disp(0x00, num[1]);
}

void scrollDisp(status target) {
	int pi = statusToPatternIndex(target);
	const int* pat = patterns[pi];
	len = lengths[pi];

	if (tc > 300) {
		tc = 0;

		if (index < len - 1) {
			onedisp(pat[index], pat[index + 1]);
			index++;
		} else {
			onedisp(0x00, 0x00);
			index = 0;
			st = status::wait;
		}
	}
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
			tc = 0;
			index = 0;

			if (country == 1) {
				st = (tsw == HIGH) ? status::yahoo : status::japan;
			} else if (country == 2) {
				st = (tsw == HIGH) ? status::apple : status::usa;
			} else {
				st = (tsw == HIGH) ? status::google : status::china;
			}
		}
		break;

	case status::japan:
		scrollDisp(status::japan);
		break;
	case status::yahoo:
		scrollDisp(status::yahoo);
		break;
	case status::usa:
		scrollDisp(status::usa);
		break;
	case status::apple:
		scrollDisp(status::apple);
		break;
	case status::china:
		scrollDisp(status::china);
		break;
	case status::google:
		scrollDisp(status::google);
		break;
	}

	if (st == status::wait) onedisp(0x00, num[country]);
	else onedisp(patterns[statusToPatternIndex(st)][index],
		(index + 1 < lengths[statusToPatternIndex(st)])
		? patterns[statusToPatternIndex(st)][index + 1]
		: 0x00);
}