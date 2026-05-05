#include <Arduino.h>
#line 1 "C:\\Users\\ao\\Desktop\\tmp\\tests\\kadai2\\kadai2.ino"
#define USE_TIMER3_ISR
#include "mono_con.h"

volatile int sw, presw;
volatile int swps = false;

volatile word in, tc;

#line 21 "C:\\Users\\ao\\Desktop\\tmp\\tests\\kadai2\\kadai2.ino"
void setup();
#line 29 "C:\\Users\\ao\\Desktop\\tmp\\tests\\kadai2\\kadai2.ino"
void loop();
#line 9 "C:\\Users\\ao\\Desktop\\tmp\\tests\\kadai2\\kadai2.ino"
ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		sw = digitalRead(_USER_CON_4PIN);
		if (sw == LOW && presw == HIGH) swps = true;
		presw = sw;
	}

	if (tc > 0) tc--;
}

void setup() {
	config_init();
	serial_init();

	sw = presw = digitalRead(_USER_CON_4PIN);
	Serial.begin(9600);
}

void loop() {
	if (swps) {
		Serial.println("pushed!");
		swps = false;
		tc = 1000;
	}

	if (tc > 0) {
		lm.color.GBR = B100;
	} else {
		lm.color.GBR = B000;
	}

	led_stepmotor(lm.b8);
}

