/**********************************************/
// 高校生ものづくりコンテスト2026全国大会
//
// 地区名: 中国地区
// 学校名: 岡山県立岡山工業高等学校
// 氏名: 青山 晃大
// 作成年月日: 2026/〇〇/〇〇
/**********************************************/

// これ以下のコメント文は後に全て削除すること

#pragma once

#define a0 A0
#define a1 A1
#define a2 A2
#define a3 A3
#define d0 2
#define d1 3
#define d2 4
#define d3 5

#define sdi 22
#define sck 23
#define lat 24
#define lr 25
#define lg 26
#define lb 27
#define m0 28
#define m1 29
#define m2 30
#define m3 31
#define buz 12
#define da 6
#define db 7
#define pho 8

#define step_rev 120
#define bl 0x00

/* 7セグメントLED start */
const uint8_t seg[16] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66,
	0x6d, 0x7d, 0x27, 0x7f, 0x6f,
	0x77, 0x7c, 0x58, 0x5e, 0x79, 0x71,
};

inline void disp(uint8_t a, uint8_t b, uint8_t c) {
	static int pa = -1, pb = -1, pc = -1;
	if (a == pa && b == pb && c == pc) return;
	pa = a; pb = b; pc = c;
	digitalWrite(lat, LOW);
	shiftOut(sdi, sck, MSBFIRST, a);
	shiftOut(sdi, sck, MSBFIRST, b);
	shiftOut(sdi, sck, MSBFIRST, c);
	digitalWrite(lat, HIGH);
}

inline void disp(int n) {
	n = constrain(n, 0, 999);
	uint8_t a = (n >= 100) ? seg[n / 100 % 10] : bl;
	uint8_t b = (n >= 10) ? seg[n / 10 % 10] : bl;
	uint8_t c = seg[n % 10];
	disp(a, b, c);
}
/* 7セグメントLED end */

/* フルカラーLED start */
class l_ {
	uint8_t p = -1;

	inline void c(uint8_t m) {
		if (m == p) return;
		p = m;
		digitalWrite(lr, (m & 1) ? HIGH : LOW);
		digitalWrite(lg, (m & 2) ? HIGH : LOW);
		digitalWrite(lb, (m & 4) ? HIGH : LOW);
	}
};

l_ l;
/* フルカラーLED end */

/* ステッピングモータ start */
class sm_ {
	const uint8_t _tbl[4] = { 0b1001, 0b1100, 0b0110, 0b0011 };
	int8_t _ph = 0;
	long   _pos = 0;
	int    _mv = 0;

	inline void _apply(uint8_t p) {
		uint8_t b = _tbl[p & 3];
		digitalWrite(m0, (b & 1) ? HIGH : LOW);
		digitalWrite(m1, (b & 2) ? HIGH : LOW);
		digitalWrite(m2, (b & 4) ? HIGH : LOW);
		digitalWrite(m3, (b & 8) ? HIGH : LOW);
	}

	inline void set(uint8_t p) {
		_ph = p & 3;
		_apply(_ph);
	}

	inline void cw() {
		_ph = (_ph + 1) & 3;
		_apply(_ph);
		_pos++;
	}

	inline void ccw() {
		_ph = (_ph + 3) & 3;
		_apply(_ph);
		_pos--;
	}

	inline void off() {
		digitalWrite(m0, LOW);
		digitalWrite(m1, LOW);
		digitalWrite(m2, LOW);
		digitalWrite(m3, LOW);
	}

	inline void mv(int n) {
		_mv += n;
	}

	inline void run() {
		if (_mv > 0) {
			cw();
			_mv--;
		} else if (_mv < 0) {
			ccw();
			_mv++;
		}
	}

	inline bool moving() {
		return _mv != 0;
	}

	inline long pos() {
		return _pos;
	}

	inline void zero() {
		_pos = 0;
	}

	inline void to(long step, bool shortest = true) {
		long d = step - _pos;
		if (shortest) {
			d %= step_rev;
			if (d > step_rev / 2) d -= step_rev;
			if (d < -step_rev / 2) d += step_rev;
		}
		_mv = (int)d;
	}

	// TODO: cw||ccw方向への最短距離
};

sm_ sm;
/* ステッピングモータ end */

/* 圧電ブザー start */
class bz_ {
	inline void on(int f) {
		tone(buz, f);
	}

	inline void on(int f, unsigned long ms) {
		tone(buz, f, ms);
	}

	inline void off() {
		noTone(buz);
	}
};

bz_ bz;
/* 圧電ブザー end */

/* DCモータ start */
class dm_ {
	inline void cw(int s) {
		analogWrite(da, s);
		digitalWrite(db, LOW);
	}

	inline void ccw(int s) {
		digitalWrite(da, LOW);
		analogWrite(db, s);
	}

	inline void br() {
		digitalWrite(da, HIGH);
		digitalWrite(db, HIGH);
	}

	inline void fr() {
		digitalWrite(da, LOW);
		digitalWrite(db, LOW);
	}
};

dm_ dm;
/* DCモータ end */

class ph_ {
private:
	volatile long cnt = 0;
	int _pv = 0;
	bool _ep = false, _en = false;

	inline int read() {
		return digitalRead(pho);
	}

	inline void scan() {
		int v = read();
		if (v != _pv) {
			if (v) {
				_ep = true;
				cnt++;
			} else {
				_en = true;
			}
		}
		_pv = v;
	}

	inline bool pos() {
		if (_ep) {
			_ep = false;
		}
		return false;
	}

	inline bool neg() {
		if (_en) {
			_en = false;
			return true;
		}
		return false;
	}

	inline void clr() {
		cnt = 0;
	}
};

ph_ ph;

/* 入力回路用 start */
class in_ {
	const uint8_t _ap[4] = { a0, a1, a2, a3 };
	const uint8_t _dp[4] = { d0, d1, d2, d3 };
	int _pv[4] = { 0, 0, 0, 0 };
	uint8_t _ep = 0, _en = 0;

	inline int a(uint8_t ch) {
		return analogRead(_ap[ch]);
	}

	inline int d(uint8_t ch) {
		return digitalRead(_dp[ch]);
	}

	inline void scan() {
		for (uint8_t c = 0; c < 4; c++) {
			int v = digitalRead(_dp[c]);
			if (v != _pv[c]) { if (v) _ep |= (1 << c); else _en |= (1 << c); }
			_pv[c] = v;
		}
	}

	inline bool pos(uint8_t ch) {
		if (_ep & (1 << ch)) {
			_ep &= ~(1 << ch); return true;
		}
		return false;
	}

	inline bool neg(uint8_t ch) {
		if (_en & (1 << ch)) {
			_en &= ~(1 << ch); return true;
		}
		return false;
	}

	inline int lvl(uint8_t ch, int n) {
		return constrain(map(analogRead(_ap[ch]), 0, 1023, 0, n), 0, n - 1);
	}

	inline int dir4(int x, int y) {
		long dx = x - 511, dy = y - 511;
		if (dx * dx + dy * dy < 20000) return -1;
		return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
	}

	inline int dir8(int x, int y) {
		long dx = x - 511, dy = y - 511;
		if (dx * dx + dy * dy < 20000) return -1;
		return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
	}

	inline int dirn(int x, int y, int n) {
		long dx = x - 511, dy = y - 511;
		if (dx * dx + dy * dy < 20000) return -1;
		return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / n) / (2 * PI / n)) % n;
	}
};

in_ in;
/* 入力回路用 end */



#if false
#include "monocon_chuugoku.h"

bool r = false;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
}

void setup() {
	init();

}

void loop() {
	if (r) {
		r = false;

	}
}
#endif
