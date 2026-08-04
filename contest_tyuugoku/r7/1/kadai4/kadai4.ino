#define USE_TIMER3_ISR
#include "mono_con.h"

int ph, sw;
int preph, presw;
bool swps = false;

int count = 0;
int bz_rep = 0, bz_wait = 0;

bool visible = true;

word in, cd, bz;

int pre_n;
void onedisp(int n, bool force = false) {
	if (force || pre_n != n) {
		disp(num[n / 10], num[n % 10]);
		pre_n = n;
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		ph = digitalRead(_USER_CON_3PIN);
		sw = digitalRead(_USER_CON_5PIN);

		if (sw == LOW && presw == HIGH) swps = true;

		presw = sw;
	}

	cd++;

	if (bz > 0) bz--;
	else noTone(BZ_PIN);

	if (bz_rep > 0) {
		if (bz_wait > 0) {
			bz_wait--;
			if (bz_wait == 0) {
				tone(BZ_PIN, 1500);
				bz = 200;
				bz_rep--;
			}
		} else if (bz == 0) {
			noTone(BZ_PIN);
			bz_wait = 150;
		}
	}
}

void setup() {
	config_init();
	serial_init();

	ph = preph = digitalRead(_USER_CON_3PIN);
	sw = presw = digitalRead(_USER_CON_5PIN);

	disp(num[0], num[0]);
}

void loop() {
	if (ph == HIGH) {
		if (preph != ph) {
			preph = ph;
			cd = 0;
			count = 0;
			visible = true;
			disp(num[0], num[0]);
			lm.color.GBR = B000;
		}

		if (cd > 1000) {
			cd = 0;
			count++;
		}

		if (swps) {
			swps = false;
			onedisp(count, true);
			visible = true;
		} else if (sw == HIGH) {
			if (ph == HIGH && count >= 3) {
				visible = false;
			}
		}

	} else {
		cd = 0;

		if (preph != ph) {
			preph = ph;
			// onedisp(count);
			visible = true;

			if (count == 10) {
				lm.color.GBR = B010;
				bz = 2000;
				tone(BZ_PIN, 1500);
			} else if (count <= 8 || count >= 12) {
				lm.color.GBR = B001;
				bz = 1000;
				tone(BZ_PIN, 800);
			} else if (count == 9 || count == 11) {
				lm.color.GBR = B101;
				bz_rep = 2;
				bz_wait = 0;
				bz = 200;
				tone(BZ_PIN, 1500);
			} else {
				lm.color.GBR = B000;
				noTone(BZ_PIN);
			}
		}

	}

	if (visible) onedisp(count);
	else disp(num[10], num[10]);
	led_stepmotor(lm.b8);
}