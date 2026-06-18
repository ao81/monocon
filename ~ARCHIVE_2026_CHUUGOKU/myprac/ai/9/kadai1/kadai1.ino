#define USE_TIMER3
#include "mono_con.h"

const int led[9] = { 0, 13, 25, 38, 50, 63, 75, 88, 100 };
const int color[3] = { B001, B100, B010 };

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;

int idx = 0;
int coloridx = 0;

bool blink = false;
bool tgl = true;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0, acc = 0, tc;

	acc += led[idx];
	if (acc >= 100) {
		acc -= 100;
		if (blink) {
			if (tc >= 500) {
				tc = 0;
				tgl = !tgl;
			}
			if (tgl) lm.led(color[coloridx]);
			else lm.led(B000);
		} else lm.led(color[coloridx]);
	} else lm.led(B000);

	lm.flush();

	if (in++ > 5) {
		in = 0;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin3);
		sw = digitalRead(pin4);
		ph = digitalRead(pin5);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	tc++;
}

void setup() {
	noInterrupts();
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin3);
	sw = presw = digitalRead(pin4);
	ph = digitalRead(pin5);

	disp(0x00, num[0]);
	interrupts();
}

void loop() {
	int i = getdir(x, y);
	static int prei = -1;

	if (i != prei) {
		if (prei == -1) {
			if (i == 0) {
				if (idx < 8) idx++;
				disp(0x00, num[idx]);
			} else if (i == 2) {
				if (idx > 0) idx--;
				disp(0x00, num[idx]);
			} else if (i == 1) {
				if (++coloridx > 2) coloridx = 0;
			}
		}
		prei = i;
	}

	blink = (tsw == HIGH);
	if (!blink) tgl = true;

	if (swps) {
		swps = false;
		idx = 0;
		coloridx = 0;
		tone(BZ_PIN, 2000, 30);
		disp(0x00, num[idx]);
	}
}
