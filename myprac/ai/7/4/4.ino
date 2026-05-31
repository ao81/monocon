#define USE_TIMER3
#include "mono_con.h"

const int spd[4] = { -1, 80, 120, 160 };
int idx = 0;
int x, y;
int sw, presw;
bool swps = false;

int angle = 0;
int move = 0;

word tc, t;

int getdir(int x_, int y_) {
	int dx = x_ - 511, dy = y_ - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
}

int getmove(int from, int to = 0) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
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
		sw = digitalRead(pin4);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
	t++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	sw = presw = digitalRead(pin4);
}

/*
7 0 1
6 - 2
5 4 3
*/

void loop() {
	int i = getdir(x, y);
	static int preid = -2;
	static int preis = -1;

	if (t >= 7) {
		t = 0;
		if (move > 0) {
			move--;
			lm.spm(CW);

		} else if (move < 0) {
			move++;
			lm.spm(CCW);

		}
	}

	if (i != preis) {
		if ((i >= 1 && i <= 3) || (i >= 5 && i <= 7)) preis = i;
	}

	if (tc >= 20) {
		tc = 0;
		if (preis >= 1 && preis <= 3) {
			lm.spm(CW);
			angle++;
		} else if (preis >= 5 && preis <= 7) {
			lm.spm(CCW);
			angle--;
		}
		angle = (angle + 120) % 120;
	}

	if (i != preid) {
		if (preid == -1) {
			if (i == 7 || i == 0 || i == 1) {
				if (idx < 3) idx++;
			} else if (i >= 3 && i <= 5) {
				if (idx > 0) idx--;
			}
		}
		preid = i;
	}

	if (idx == 0) stop();
	else {
		cw(spd[idx]);
	}

	if (swps) {
		swps = false;
		idx = 0;
		i = preis = preid = -1;
		move = getmove(angle);
		angle = 0;
	}

	disp(0x00, num[idx]);
	lm.flush();
}
