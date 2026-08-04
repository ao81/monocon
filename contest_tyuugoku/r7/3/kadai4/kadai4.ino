#define USE_TIMER3
#include "mono_con.h"

int ph, preph;
bool phps = false;

int n = 0;
int cnt = 0;
word tc, bz;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;

		ph = digitalRead(pin_3);
		if (ph == HIGH && preph == LOW) phps = true;
		preph = ph;
	}

	tc++;

	if (bz > 0) {
		bz--;
		if (bz <= 0) noTone(BZ_PIN);
	}
}

void setup() {
	config_init();
	serial_init();

	ph = digitalRead(pin_3);

	disp(num[0], num[0]);
}

typedef enum {
	WAIT,
	MOVE,
	RES,
	END,
} status;
status st = WAIT;

void loop() {
	if (phps) {
		phps = false;
		st = WAIT;
	}

	switch (st) {
	case WAIT:
		lm.led(B000);
		noTone(BZ_PIN);

		if (ph == HIGH) {
			st = MOVE;
			tc = 0;
			cnt = 0;
		}

		break;
	case MOVE:
		if (ph == HIGH) {
			if (tc == 0) break;
			cnt = (tc + 999) / 1000 - 1;
			if (cnt < 3) disp(num[cnt / 10], num[cnt % 10]);
			else disp(0x00, 0x00);
		} else {
			st = RES;
			n = 0;
		}

		break;
	case RES:
		disp(num[cnt / 10], num[cnt % 10]);

		if (cnt == 10) {
			lm.led(B010);
			bz = 2000;
			tone(BZ_PIN, 2000);
			st = END;
		} else if (cnt == 9 || cnt == 11) {
			lm.led(B101);
			if (tc >= 200) {
				tc = 0;
				if (n++ < 3) tone(BZ_PIN, 2000, 100);
				else st = END;
			}
		} else {
			lm.led(B001);
			bz = 1000;
			tone(BZ_PIN, 800);
			st = END;
		}

		break;
	case END: break;
	}

	lm.update();
}
