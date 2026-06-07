#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int pretsw, presw;
bool swps = false;
bool r = true;

int setn = 1;
int setnow = 1;

int undoucnt = 0;
int kyukeicnt = 0;

bool tgl = true;

word cd = 0;

typedef enum {
	set,
	undou,
	kyukei,
	waitinit,
	stop,
} status;
status st = set;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static int in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}

	if (cd > 0) cd--;
}

void setup() {
	config_init();
	serial_init();

	tsw = pretsw = digitalRead(pin5);
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

	if (tsw != pretsw) {
		static status remst;
		static word remcd;

		pretsw = tsw;
		if (tsw == HIGH) {
			remst = st;
			remcd = cd;
			st = stop;
		} else {
			st = remst;
			cd = remcd;
		}
	}

	switch (st) {
	case set:
		lm.led(B101);

		{
			int i = getdir(x, y);
			static int prei = -1;
			if (i != prei) {
				if (prei == -1) {
					static const int a[4] = { 1, 0, -1, 0 };
					setn = constrain(setn + a[i], 1, 9);
				}
				prei = i;
			}
		}

		if (swps) {
			swps = false;
			st = undou;
			cd = 0;
			undoucnt = 10;
			setnow = 1;
		}

		disp(num[setn], 0x00);

		break;
	case undou:
		lm.led(B001);

		if (cd <= 0) {
			cd = 1000;
			if (undoucnt > 0) {
				disp(num[setnow], num[--undoucnt]);
			} else {
				st = kyukei;
				cd = 0;
				kyukeicnt = 6;
				tone(BZ_PIN, 1200, 100);
			}
		}

		break;
	case kyukei:
		lm.led(B010);

		if (cd <= 0) {
			cd = 1000;
			if (kyukeicnt > 0) {
				disp(num[setnow], num[--kyukeicnt]);
			} else {
				if (++setnow >= setn) {
					st = waitinit;
					tgl = true;
					cd = 0;
					tone(BZ_PIN, 2000);
				} else {
					st = undou;
					tone(BZ_PIN, 1200, 100);
					cd = 0;
					undoucnt = 10;
				}
			}
		}

		break;
	case waitinit:
		if (cd <= 0) {
			cd = 500;
			lm.led(tgl ? B100 : B000);
			tgl = !tgl;
		}

		if (swps) {
			swps = false;
			lm.led(B000);
			noTone(BZ_PIN);
			st = set;
			setn = 1;
		}

		disp(0x00, 0x00);

		break;
	case stop:
		break;
	}

	lm.flush();
}
