// 15:24

#define USE_TIMER3
#include "mono_con.h"

const int speed = 100;
const int ledpara[3] = { B001, B100, B010 };
const int ledspeed[3] = { 500, 300, 150 };

int x, y;
int tsw, sw, ph;
int presw, preph;
bool swps = false, phps = false;

int n = 0;
bool tgl = true;
int idx = 0;
int spdidx = 0;
bool dir = true;

word dm, led, seg;

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

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);

		if (sw == HIGH && presw == LOW) swps = true;
		if (ph == LOW && preph == HIGH) phps = true;

		presw = sw;
		preph = ph;
	}

	if (dm > 0) dm--;
	if (led > 0) led--;
	if (seg > 0) seg--;
}

void setup() {
	config_init();
	serial_init();

	x = analogRead(pin2);
	y = analogRead(pin1);
	tsw = digitalRead(pin5);
	sw = presw = digitalRead(pin4);
	ph = preph = digitalRead(pin3);

	Serial.begin(9600);
}

void loop() {
	if (swps) {
		swps = false;
		n = 0;
		tgl = true;
		idx = 0;
		spdidx = 0;
		dir = true;
		dm = led = seg = 0;
	}

	if (dm <= 0) {
		if (tsw == LOW) dm = 1000;
		else dm = 500;
		if (tgl) cw();
		else stop();
		tgl = !tgl;
	}

	static int prei = -1;
	int i = getdir(x, y);
	/*	- 0 -
		3 - 1
		- 2 -	*/

	if (prei != i) {
		if (prei == -1) {
			Serial.println(i);
			if (i == 0) {
				if (spdidx < 2) spdidx++;
			} else if (i == 2) {
				if (spdidx > 0) spdidx--;
			}
		}
		prei = i;
	}

	if (led <= 0) {
		led = ledspeed[spdidx];
		lm.led(ledpara[idx]).flush();
		if (++idx > 2) idx = 0;
	}

	if (phps) {
		phps = false;
		dir = !dir;
	}

	if (seg <= 0) {
		seg = 200;
		disp(0x00, num[n]);
		if (dir) {
			if (++n > 9) n = 0;
		} else {
			if (--n < 0) n = 9;
		}
	}
}
