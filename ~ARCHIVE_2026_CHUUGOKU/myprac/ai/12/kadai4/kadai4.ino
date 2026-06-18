// 途中

#define USE_TIMER3
#include "mono_con.h"

int x, y;
int tsw, sw, ph;
int presw;
bool swps = false;
bool r = true;

bool mode = false;
int cnt = 0;

word tc = 0;
bool tgl = true;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
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

		if (!mode) {
			if (tsw == LOW) {
				lm.led(B100);
				if (ph == HIGH) {
					mode = true;
					tgl = true;
					tone(BZ_PIN, 2000);
					if (cnt < 9) cnt++;
				}

			} else {
				lm.led(B000);
			}

			swps = false;

		} else {
			if (tc >= 500) {
				tc = 0;
				lm.led(tgl ? B001 : B000);
				tgl = !tgl;
			}

			if (swps) {
				swps = false;
				mode = false;
				noTone(BZ_PIN);
			}
		}

	disp(0x00, num[cnt]);
	lm.flush();
}
