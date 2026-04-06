/** - 本来ロータリーエンコーダで入力するのをsw2で入力するように変更
 *  - sw2でリセットする処理を削除
 */

#define USE_TIMER3_ISR
#include "mono_con.h"

const int years[2] = { 1989, 2019 }; // 平成/令和　元年

int year = 1, month = 1, day = 1;
bool isReiwa;

int angelNum = 0;

int disp_num = 1;

int tsw, sw1, sw2;
int pretsw, presw1, presw2;
bool sw1ps = false, sw2ps = false;

word in;

const int ledParam[4] = { B001, B100, B010, B111 };

typedef enum {
	IN_YEAR,
	IN_MONTH,
	IN_DAY,
	OUT_RESULT,
} Status;
Status st = IN_YEAR;

int pre_n;
void onedisp(int n) {
	if (pre_n != n) {
		disp(num[n / 10], num[n % 10]);
		pre_n = n;
	}
}

int month_day(int value) {
	switch (value) {
	case 1: case 3: case 5: case 7: case 8: case 10: case 12:
		return 31;
	case 4: case 6: case 9: case 11:
		return 30;
	default:
		return 28;
	}
}

int toSeireki(int value, bool reiwa) {
	if (reiwa) return (years[1] + value - 1);
	else return (years[0] + value - 1);
}

int digitSum(int value) {
	int res = 0;
	while (value > 0) {
		res += value % 10;
		value /= 10;
	}
	return res;
}

int getAngelNum(int n) {
	if (n < 10 || n % 11 == 0) return n;
	return getAngelNum(digitSum(n));
}

ISR(TIMER3_COMPA_vect) {
	if (++in > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_4PIN);
		sw1 = digitalRead(_USER_CON_5PIN);
		sw2 = digitalRead(_USER_CON_3PIN);

		if (sw1 == LOW && presw1 == HIGH) sw1ps = true;
		if (sw2 == LOW && presw2 == HIGH) sw2ps = true;

		presw1 = sw1;
		presw2 = sw2;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(_USER_CON_4PIN);
	sw1 = presw1 = digitalRead(_USER_CON_5PIN);
	sw2 = presw2 = digitalRead(_USER_CON_3PIN);

	isReiwa = tsw;

	lm.color.GBR = ledParam[0];
	disp(num[0], num[0]);
}

void loop() {
	if (sw1ps) {
		sw1ps = false;
		if (st < OUT_RESULT) st = (Status)(st + 1);
		else st = IN_YEAR;
		lm.color.GBR = ledParam[(int)st];
	}

	switch (st) {
	case IN_YEAR:
		if (sw2ps) {
			sw2ps = false;
			if (isReiwa) {
				if (++year > 3) year = 1;
			} else {
				if (++year > 31) year = 1;
			}
		}

		if (tsw != pretsw) {
			pretsw = tsw;
			isReiwa = !isReiwa;
			year = 1;
		}

		disp_num = year;

		break;
	case IN_MONTH:
		if (sw2ps) {
			sw2ps = false;
			if (++month > 12) month = 1;
		}

		disp_num = month;

		break;
	case IN_DAY:
		if (sw2ps) {
			sw2ps = false;
			if (++day > month_day(month)) day = 1;
		}

		disp_num = day;

		break;
	case OUT_RESULT:
		angelNum = digitSum(toSeireki(year, isReiwa)) + digitSum(month) + digitSum(day);
		disp_num = getAngelNum(angelNum);

		break;
	}

	onedisp(disp_num);
	led_stepmotor(lm.b8);
}