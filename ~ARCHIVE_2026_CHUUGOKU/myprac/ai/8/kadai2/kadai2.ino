// 16:49

#define USE_TIMER3
#include "mono_con.h"

const int spd[3] = { 160, 120, 80 };

int x, y;
int tsw, sw, ph;
int preph, presw;
bool phps = false, swps = false;

int n = 0;
int cnt = 0;

bool on = true;
bool first = true;

typedef enum {
	SET,
	MOVE,
} status;
status st = SET;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

void cw(int speed) {
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
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	disp(num[0], num[0]);
}

void loop() {
	switch (st) {
	case SET: {
		lm.led(B000);

		int i = getdir(x, y);
		static int prei = -1;
		if (i != prei) {
			if (prei == -1) {
				if (i == 0) {
					n++;
				} else if (i == 1) {
					n += 10;
				} else if (i == 2) {
					n--;
				} else {
					n -= 10;
				}
			}
			n = constrain(n, 0, 99);
			prei = i;
		}

		if (swps) {
			swps = false;
			st = MOVE;
			lm.led(B001);
			phps = false;
			cnt = 0;
			first = true;
		}

		disp(num[n / 10], num[n % 10]);

		break;
	}
	case MOVE:
		on = (tsw == HIGH);

		if (on) {
			int diff = n - cnt;

			if (diff > 0) lm.led(B001);

			if (phps) {
				phps = false;
				if (diff > 0) {
					if (cnt < 99) cnt++;
				}
			}

			if (diff >= 10) {
				cw(spd[0]);
			} else if (diff >= 4) {
				cw(spd[1]);
			} else if (diff >= 1) {
				cw(spd[2]);
			} else {
				stop();
				lm.led(B100);
				if (first) {
					first = false;
					tone(BZ_PIN, 2000, 1000);
				}
			}
		} else {
			stop();
			lm.led(B000);
		}

		disp(num[cnt / 10], num[cnt % 10]);

		break;
	}

	lm.flush();
}
