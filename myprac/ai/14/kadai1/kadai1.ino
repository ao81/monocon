#define USE_TIMER3
#include "mono_con.h"

const int seggate[4] = { 0x77, 0x7c, 0x58, 0x5e };
int gate = 0;

const int speed = 100;

int x, y, tsw, sw, ph;
bool r = true;

int inx = 0, iny = 0, f = 0;

int getdir(int xx, int yy) {
	int dx = xx - 511, dy = yy - 511;
	if ((long)dx * dx + (long)dy * dy < 20000) return -1;
	return (int)((atan2((double)dx, (double)dy) + 2 * PI + PI / 4) / (PI / 2)) % 4;
}

void cw() {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (r) {
		r = false;
		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);
	}

	int i = getdir(x, y);
	static int prei = -1;

	if (i != prei) {
		if (prei == -1) {
			gate = i;
		}
		prei = i;
	}

	inx = (tsw == HIGH ? 1 : 0);
	iny = (ph == HIGH ? 1 : 0);

	if (gate == 0) f = inx & iny;
	else if (gate == 1) f = inx | iny;
	else if (gate == 2) f = inx ^ iny;
	else f = !inx;

	static int pref = 1;

	if (f != pref) {
		pref = f;
		if (f == 1) tone(BZ_PIN, 2000, 50);
	}

	if (f) {
		cw();
		lm.led(B100);
	} else {
		stop();
		lm.led(B001);
	}

	lm.flush();
	disp(seggate[gate], 0x00);
}
