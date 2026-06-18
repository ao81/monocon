// 自動巻線機（コイルワインダー）の制御システム
// 途中まで

#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;
int x, y;
int tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
int cnt = 0;

bool on = false;

word bz, tc;

int getidx(int yy) {
	int dy = yy - 511;
	if (abs(dy) < 200) return 0;
	if (dy > 0) return 1;
	else return 2;
}

void cw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void ccw() {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word i = 6;
	if (i++ > 5) {
		i = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}

	if (bz > 0) {
		bz--;
		if (bz == 0) noTone(BZ_PIN);
	}

	if (tc > 0) tc--;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	pretsw = !tsw;
}

void loop() {
	if (tsw != pretsw) {
		pretsw = tsw;
		cnt = 0;
		swps = phps = false;
		on = false;
	}

	if (tsw == LOW) {
		lm.led(B100);

		static int prei = -1;
		int i = getidx(y);

		if (i != prei) {
			if (prei == 0)
		}

		if (i == 1) cw();
		else if (i == 2) ccw();
		else stop();

		if (phps) {
			phps = false;
			if (++cnt > 99) cnt = 0;
		}

	} else {
		lm.led(B010);
		static int precnt = 0;

		if (swps) {
			swps = false;
			if (!on) {
				on = true;
				precnt = cnt;
			}
		}

		if (on) {
			cw();

			if (phps) {
				phps = false;
				if (++cnt >= precnt + 10) {
					on = false;
					tone(BZ_PIN, 523);
					bz = 500;
				}
			}
		} else {
			stop();
			phps = false;
		}

		if (cnt > 99) {
			cnt -= 100;
			precnt -= 100;
		}
	}

	disp(num[cnt / 10], num[cnt % 10]);
}
