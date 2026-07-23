#define USE_TIMER3
#include "mono_con.h"

const int seg[3] = { 0x77, 0x7c, 0x58 };

int buf[10] = { 0 };
int head = 0;

int x, y, tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;
bool r = true;

int mode = 0;
int f = 0;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
}

void setup() {
	config_init();
	serial_init();

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

	int i = getdir(x, y);
	static int prei = -1;

	if (tsw == LOW) {
		lm.led(B000);

		if (i != prei) {
			if (prei == -1) {
				static const int a[] = { 1, 10, -1, -10 };
				if (head <= 9) buf[head] = constrain(buf[head] + a[i], 0, 99);
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			if (head <= 9) {
				tone(BZ_PIN, 2000, 50);
				head++;
			}
		}

		if (ph) disp(head < 10 ? num[head] : num[9], 0x00);
		else if (head <= 9) disp(num[buf[head] / 10], num[buf[head] % 10]);
		else disp(num[buf[9] / 10], num[buf[9] % 10]);

		mode = 0;
		phps = false;

	} else {
		if (i != prei) {
			if (prei == -1 && i != 3) {
				mode = i;
			}
			prei = i;
		}

		if (mode == 0) {
			lm.led(B100);
			int sum = 0;
			for (int i = 0; i < 10; i++) sum += buf[i];
			f = sum / 10;
		} else if (mode == 1) {
			lm.led(B001);
			int mx = 0;
			for (int i = 0; i < 10; i++) if (buf[i] > mx) mx = buf[i];
			f = mx;
		} else {
			lm.led(B010);
			int mn = 99;
			for (int i = 0; i < 10; i++) if (buf[i] < mn) mn = buf[i];
			f = mn;
		}

		swps = false;
		if (phps) {
			phps = false;
			for (int i = 0; i < 10; i++) buf[i] = 0;
		}

		disp(num[f / 10], num[f % 10]);
		lm.flush();
	}
}
