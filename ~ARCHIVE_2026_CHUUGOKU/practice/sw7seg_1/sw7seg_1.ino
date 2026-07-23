#define USE_TIMER3_ISR
#include "mono_con.h"

const int D2 = 18;
int SW = HIGH, prevSW = HIGH;

word sTime;

ISR (TIMER3_COMPA_vect) {
  if (sTime > 5) {
    sTime = 0;

    SW = digitalRead(D2);
  }

  sTime++;
}

void setup() {
	config_init();
	serial_init();
	// disp(num[8], num[8]);
}

int status = 1;

void loop() {
	if (SW == LOW && prevSW == HIGH) {
		status = !status;
	}

	if (status) {
		disp(num[8], num[8]);
	} else {
		disp(num[10], num[10]);
	}

	prevSW = SW;
}