#define USE_TIMER3
#include "mono_con.h"

const int angles[2] = { 0, 20 };
int agl = 0, mv = 0;
word tc, cd, dn, tt;

const int speed = 100;
int cnt[2] = { 0 };
bool tgl = true;

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

bool move = false;
bool mx = false;

int c = 0;
bool force = false;

void cw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

int getmove(int from, int to) {
	return to - from;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}

	tc++;
	tt++;

	if (cd > 0) {
		cd--;
		if (cd == 0) lm.led(B000);
	}

	if (ph == LOW) {
		if (dn > 0) dn--;
	} else dn = 2000;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == LOW && preph == HIGH) phps = true;

		presw = sw;
		preph = ph;
	}

	if (tsw != pretsw) {
		pretsw = tsw;
		if (tsw == LOW) {
			stop();
			cnt[0] = cnt[1] = 0;
			force = mx = move = false;
			tgl = true;
			mv = getmove(agl, 0);
		}
	}

	if (swps) {
		swps = false;
		move = !move;
	}

	if (force) {
		stop();

		if (tc >= 400) {
			tc = 0;

			if (tgl) lm.led(B001);
			else lm.led(B000);

			tgl = !tgl;
		}

		if (swps) {
			swps = false;
			force = false;
		}

	} else if (mx) {
		stop();

		if (tc >= 400) {
			tc = 0;
			if (tgl) lm.led(cnt[0] == 9 ? B100 : B010);
			else lm.led(B000);
			tgl = !tgl;

			if (c-- > 0) {
				tone(BZ_PIN, 2000, 200);
			}
		}

	} else if (move) {
		cw();

		if (phps) {
			phps = false;

			if (tgl) {
				if (cnt[0] < 9) cnt[0]++;
				lm.led(B100);
			} else {
				if (cnt[1] < 9) cnt[1]++;
				lm.led(B010);
			}

			cd = 100;
			tgl = !tgl;

			if (!tgl) mv = getmove(agl, angles[1]);
			else mv = getmove(agl, angles[0]);
		}

		if (cnt[0] >= 9 || cnt[1] >= 9) {
			mx = true;
			tgl = true;
			tc = 400;
			c = 2;
			return;
		}

	} else {
		stop();
		phps = false;
	}

	if (tt >= 8) {
		tt = 0;

		if (mv > 0) {
			lm.spm(CW);
			mv--;
			agl++;
		} else if (mv < 0) {
			lm.spm(CCW);
			mv++;
			agl--;
		}
	}

	disp(num[cnt[0] % 10], num[cnt[1] % 10]);
	lm.flush();
}
