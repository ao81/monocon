// 24:26

#define USE_TIMER3_ISR
#include "mono_con.h"

int ph, sw;
int preph, presw;
int swps = false;

bool visible = true;
int count = 0;

word in, cd, bz;
word bz_pause = 0, bz_rep = 0, bz_hz = 0;

void onedisp(int n) {
	static int pren = -2;
	if (n != pren) {
		if (n == -1) disp(num[16], num[16]);
		else disp(num[n / 10], num[n % 10]);
		pren = n;
	}
}

ISR (TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		ph = digitalRead(_USER_CON_3PIN);
		sw = digitalRead(_USER_CON_5PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	if (bz > 0) {
		bz--;
		if (bz == 0) {
			noTone(BZ_PIN);
			if (bz_rep > 0) bz_pause = 100;
		}
	} else if (bz_pause > 0) {
		bz_pause--;
		if (bz_pause == 0 && bz_rep > 0) {
			tone(BZ_PIN, bz_hz);
			bz = 100;
			bz_rep--;
		}
	} else noTone(BZ_PIN);

	cd++;
}

void setup() {
	config_init();
	serial_init();

	disp(num[0], num[0]);
}

void loop() {
	if (ph == HIGH) {
		if (ph != preph) {
			preph = ph;
			count = 0;
			cd = 0;
			visible = true;
			disp(num[0], num[0]);
			lm.color.GBR = B000;
			led_stepmotor(lm.b8);
		}

		if (cd > 1000) {
			cd = 0;
			count++;
		}

		if (sw == LOW) visible = true;
		else if (count > 3) visible = false;
		else visible = true;

	} else {
		if (ph != preph) {
			preph = ph;
			visible = true;
			bz_rep = 0;

			if (count == 10) {
				lm.color.GBR = B010;
				tone(BZ_PIN, 1500);
				bz = 2000;
			} else if (count == 9 || count == 11) {
				lm.color.GBR = B101;
				bz_hz = 1500;
				tone(BZ_PIN, bz_hz);
				bz = 100;
				bz_rep = 2;
			} else {
				lm.color.GBR = B001;
				tone(BZ_PIN, 800);
				bz = 1000;
			}

			led_stepmotor(lm.b8);
		}
	}

	if (visible) onedisp(count);
	else onedisp(-1);
}