#define USE_TIMER3
#include "mono_con.h"

const int a[8][2] = {
	{ 0, 1 },
	{ 1, 1 },
	{ 1, 0 },
	{ 1, -1 },
	{ 0, -1 },
	{ -1, -1 },
	{ -1, 0 },
	{ -1, 1 },
};

int x, y, tsw, sw, ph;
int pretsw, presw, preph;
bool swps = false, phps = false;
bool r = true;

int xp = 0, yp = 0;
int xr = -1, yr = -1;

bool ok = true;

word cd = 0, tc = 0;

int getdir(int xx, int yy) {
	long dx = xx - 511, dy = yy - 511;
	if (dx * dx + dy * dy < 20000) return -1;
	return (int)((atan2(dx, dy) + 2 * PI + PI / 8) / (PI / 4)) % 8;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (++in > 5) {
		in = 0;
		r = true;
	}

	if (cd > 0) cd--;
	tc++;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	randomSeed(analogRead(0));
}

void loop() {
	if (r) {
		r = false;

		y = 1023 - analogRead(pin2);
		x = 1023 - analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == LOW && preph == HIGH) phps = true;

		presw = sw;
		preph = ph;
	}

	int i = getdir(x, y);
	static int prei = -1;

	if (i != prei) {
		if (prei == -1) {
			xp = constrain(xp + a[i][0], 0, 9);
			yp = constrain(yp + a[i][1], 0, 9);
		}
		prei = i;
	}

	if (swps) {
		swps = false;
		xr = random(0, 10);
		yr = random(0, 10);
		cd = 1000;
		ok = false;
	}

	static int cnt = 0;

	if (cd > 0) {
		disp(num[xr], num[yr]);
		lm.led(B101);
	} else {
		if (tsw){
			if (xp == -1 || yp == -1) disp(0x00, 0x00);
			else disp(num[xr], num[yr]);
		} else disp(num[xp], num[yp]);
		if (xr == -1 || yr == -1) lm.led(B000);
		else {
			if (!ok) {
				int diff = abs(xr - xp) + abs(yr - yp);
				if (diff >= 4) lm.led(B001);
				else if (diff >= 1) lm.led(B101);
				else {
					lm.led(B100);
					ok = true;
					cnt = 2;
					tc = 200;
				}
			}
		}
	}

	if (tc >= 200) {
		tc = 0;
		if (cnt > 0) {
			cnt--;
			tone(BZ_PIN, 2000, 100);
		}
	}

	if (phps) {
		phps = false;
		xp = yp = 0;
	}

	lm.flush();
}
