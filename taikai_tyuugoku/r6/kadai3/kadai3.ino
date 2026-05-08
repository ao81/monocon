// 5:35
// sw2無し

#define USE_TIMER3_ISR
#include "mono_con.h"

int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int val = 0;

word in;

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		tsw = digitalRead(_USER_CON_5PIN);
		sw = digitalRead(_USER_CON_4PIN);
		ph = digitalRead(_USER_CON_3PIN);

		if (tsw == HIGH && sw == HIGH && presw == LOW) swps = true;
		if (ph == HIGH && preph == LOW) phps = true;

		presw = sw;
		preph = ph;
	}
}

void setup() {
	config_init();
	serial_init();

	tsw = digitalRead(_USER_CON_5PIN);
	sw = presw = digitalRead(_USER_CON_4PIN);
	ph = preph = digitalRead(_USER_CON_3PIN);
}

void loop() {
	if (tsw == HIGH) {
		if (swps) {
			swps = false;
			if (val < 5) val++;
			// if (val > -5) val--;
		}
	}

	if (phps) {
		phps = false;
		val = 0;
	}

	if (val >= 0) disp(0x00, num[val]);
	else disp(0x40, num[abs(val)]);
}