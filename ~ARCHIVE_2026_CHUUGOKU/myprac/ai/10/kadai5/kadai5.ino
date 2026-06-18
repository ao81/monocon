#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool input = true;

bool move = false;
int n, yosou = 0;

typedef enum {
	INIT,
	GAME,
	END,
} status;
status st = INIT;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		input = true;
	}
}

void setup() {
	config_init();
	serial_init();
	randomSeed(analogRead(A15));
	sw = presw = digitalRead(pin4);
}

void loop() {
	if (input) {
		input = false;
		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;

		presw = sw;
	}

	switch (st) {
	case INIT:
		if (swps) {
			swps = false;
			st = GAME;
			n = random(0, 100);
			yosou = 0;
			disp(num[0], num[0]);
			break;
		}
		disp(0x00, 0x00);
		lm.led(B000).flush();

		break;
	case GAME: {
		int i = getdir(x, y);
		static int prei = -1;

		if (i != prei) {
			if (prei == -1) {
				if (i == 0) yosou++;
				else if (i == 1) yosou += 10;
				else if (i == 2) yosou--;
				else yosou -= 10;
				yosou = constrain(yosou, 0, 99);
				disp(num[yosou / 10], num[yosou % 10]);
			}
			prei = i;
		}

		if (swps) {
			swps = false;
			if (yosou < n) {
				lm.led(B010);
				tone(BZ_PIN, 800, 1000);
			} else if (yosou > n) {
				lm.led(B001);
				tone(BZ_PIN, 2000, 1000);
			} else {
				lm.led(B100);
				tone(BZ_PIN, 2000, 3000);
				st = END;
			}
			lm.flush();
		}

		break;
	}
	case END:
		if (swps) {
			swps = false;
			st = INIT;
		}

		break;
	}
}
