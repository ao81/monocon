#define USE_TIMER3
#include "mono_con.h"

const int led[3] = { B001, B100, B010 };
const int seg[3] = { 0x77, 0x7c, 0x58 };
int vote[3] = { 0 };

int x, y, tsw, sw, ph;
int presw;
bool swps = false;
bool r = true;

bool draw = false;
bool open = false;

bool tgl = true;

word tc = 0, cd = 0;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

int winner() {
	int idx = 0;
	for (int i = 1; i < 3; i++) {
		if (vote[i] > vote[idx]) idx = i;
	}

	int cnt = 0;
	for (int i = 0; i < 3; i++) {
		if (vote[i] == vote[idx]) cnt++;
	}

	return (cnt > 1) ? -1 : idx;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
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

		presw = sw;
	}

	int i = getdir(x, y);
	static int prei = -1;
	if (i != prei) {
		if (!tsw && prei == -1 && i != 3) {
			if (vote[i] < 99) vote[i]++;
			lm.led(led[i]);
			cd = 100;
		}
		prei = i;
	}

	if (swps) {
		swps = false;

		if (!open) {
			int win = winner();
			if (win == -1) {
				draw = true;
				tone(BZ_PIN, 800, 1000);
				tgl = true;
			} else {
				disp(seg[win], num[vote[win] % 10]);
			}

			open = true;
		} else {
			vote[0] = vote[1] = vote[2] = 0;
			open = false;
			draw = false;
			disp(0x00, 0x00);
		}
	}

	if (draw) {
		if (tc >= 500) {
			tc = 0;
			lm.led(tgl ? B111 : B000);
			tgl = !tgl;
		}
	} else if (cd <= 0) lm.led(B000);
	
	lm.flush();
}
