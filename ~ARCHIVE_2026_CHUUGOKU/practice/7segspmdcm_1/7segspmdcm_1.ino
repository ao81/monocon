#define USE_TIMER3_ISR
#include "mono_con.h"

const int D2 = 18; //タクトスイッチをデジタルD2へ接続。
const int D3 = 19; //トグルスイッチをデジタルD3へ接続。

int SW = 0;
int TSW = 0;
int value = 0;
int prevSW = 0;
int phase = 0;

word sTime, spm;

ISR (TIMER3_COMPA_vect) {
	if (sTime > 5) {
		sTime = 0;

		SW = digitalRead(D2);
		TSW = digitalRead(D3);
	}

	sTime++;
	spm++;
}

void DMcw(int speed) {
	analogWrite(FIN_PIN, speed);
	digitalWrite(RIN_PIN, LOW);
}

void DMstop(void) {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void setup() {
	config_init();
	serial_init();
	disp(num[10], num[value]);
}

void loop() {
	if (TSW == HIGH) {
		if (spm > 10) {
			spm = 0;
			lm.bit.SM = stepm_init(phase);
			led_stepmotor(lm.b8);
			if (++phase > 3) phase = 0;
		}
	}

	if (SW == LOW && prevSW == HIGH) {
		if (++value > 9) value = 0;
		disp(num[10], num[value]);
	}
	prevSW = SW;

	if (value >= 2 && value <= 9) {
		DMcw(80);
	} else {
		DMstop();
	}
}