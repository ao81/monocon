// (3)の大当たりの処理まで

#define USE_TIMER3
#include "mono_con.h"

const int speed = 128;

int y;
int tsw, sw, ph;
int pretsw, presw;
bool swps = false;

bool t = true;
bool dir = true;
int n = 0;
bool move = true;
int cnt = 0;
int angle = 0;

word tc, sp, bz, dm;

int getidx(int yy) {
	return constrain(map(yy, 0, 1023, 0, 3), 0, 2);
}

void cw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
	sp++;
	if (bz > 0) {
		bz--;
		if (bz == 0) noTone(BZ_PIN);
	}
	if (dm > 0) {
		dm--;
		if (dm == 0) stop();
	}
}

void setup() {
	config_init();
	serial_init();

	y = analogRead(pin1);
	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = digitalRead(pin3);

	pretsw = !tsw;
}

void loop() {
	if (pretsw != tsw) {
		pretsw = tsw;
		if (tsw == LOW) {
			t = true;
			tc = 1000;
			disp(0x00, 0x00);
			lm.spm(STOP);

		} else {
			lm.led(B010);
			move = true;
		}
	}

	if (tsw == LOW) {
		if (tc >= 1000) {
			tc = 0;
			if (t) lm.led(B100);
			else lm.led(B000);
			t = !t;
		}

		lm.flush();
	} else {
		if (move) {
			int i = getidx(y); // 上2 中1 下0
			static int prei = -1;
			if (i == 2 || prei == 2) {
				prei = 2;
				if (tc >= 100) {
					tc = 0;
					if (++n > 9) n = 0;
				}
				if (sp >= 7) {
					sp = 0;
					lm.spm(CW);
				}
			}
			if (i == 0 || prei == 0) {
				prei = 0;
				if (tc >= 500) {
					tc = 0;
					if (++n > 9) n = 0;
				}
				if (sp >= 30) {
					sp = 0;
					lm.spm(CW);
				}
			}
		} else {
			if (n == 7) {
				if (tc >= 100) {
					tc = 0;
					if (t) lm.led(B100);
					else lm.led(B000);
					t = !t;
				}
				if (sp >= 40) {
					sp = 0;
					if (cnt < 3) {
						if (dir) {
							lm.spm(CW);
							if (--angle <= 0) {
								angle = 15;
								dir = false;
							}
						} else {
							lm.spm(CCW);
							if (--angle <= 0) {
								angle = 15;
								dir = true;
								cnt++;
							}
						}
					}
				}
			}
		}

		if (swps) {
			swps = false;
			move = false;
			t = true;

			if (n == 7) {
				tone(BZ_PIN, 523);
				bz = 200;
				cw();
				dm = 3000;
				cnt = 0;
				t = true;
				angle = 15;
				dir = true;
			}
		}

		disp(0x00, num[n]);
	}

	lm.flush();
}
