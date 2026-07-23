#define USE_TIMER3_ISR
#include "mono_con.h"

const int led[5] = { B000, B100, B001, B010, B110 };
const int speed = 150;

int x, y;
int phase = 0;

word tc;

int getidx(int xx, int yy) {
	const int dz = 300;
	int xdi = xx - 511;
	int ydi = yy - 511;

	if (abs(xdi) < dz && abs(ydi) < dz) return 0;
	if (abs(xdi) < abs(ydi)) return (xdi < 0 ? 1 : 2);
	else return (ydi < 0 ? 4 : 3);
}

void dcw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void dccw() {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, speed);
}

void dstop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void scw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
}

void sccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		x = analogRead(A1);
		y = analogRead(A2);
	}

	tc++;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(A1);
	y = analogRead(A2);
}

void loop() {
	int i = getidx(x, y);

	lm.color.GBR = led[i];
	if (i == 1) { dcw(); tone(BZ_PIN, 1200); }
	else if (i == 2) { dccw(); tone(BZ_PIN, 800); }
	else if (i == 3 || i == 4 ) {
		if (tc >= 30) {
			tc = 0;
			if (i == 3) scw();
			else if (i == 4) sccw();
		}
	} else {
		dstop();
		noTone(BZ_PIN);
	}

	led_stepmotor(lm.b8);
	disp(0x00, num[phase]);
}
