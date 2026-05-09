#include <Arduino.h>
#line 1 "C:\\Users\\ao\\Desktop\\monokon\\myprac\\ai\\1\\kadai3a\\kadai3a.ino"
#define USE_TIMER3_ISR
#include "mono_con.h"

int y, phase = 0;
word in, tc;

#line 7 "C:\\Users\\ao\\Desktop\\monokon\\myprac\\ai\\1\\kadai3a\\kadai3a.ino"
int getidx(int n);
#line 11 "C:\\Users\\ao\\Desktop\\monokon\\myprac\\ai\\1\\kadai3a\\kadai3a.ino"
void cw();
#line 17 "C:\\Users\\ao\\Desktop\\monokon\\myprac\\ai\\1\\kadai3a\\kadai3a.ino"
void ccw();
#line 31 "C:\\Users\\ao\\Desktop\\monokon\\myprac\\ai\\1\\kadai3a\\kadai3a.ino"
void setup();
#line 37 "C:\\Users\\ao\\Desktop\\monokon\\myprac\\ai\\1\\kadai3a\\kadai3a.ino"
void loop();
#line 7 "C:\\Users\\ao\\Desktop\\monokon\\myprac\\ai\\1\\kadai3a\\kadai3a.ino"
int getidx(int n) {
	return map(n, 0, 1023, 0, 3);
}

void cw() {
	if (--phase < 0) phase = 3;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

void ccw() {
	if (++phase > 3) phase = 0;
	lm.color.SM = stepm_init(phase);
	led_stepmotor(lm.b8);
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		y = analogRead(A2);
	}
	tc++;
}

void setup() {
	config_init();
	serial_init();
	y = analogRead(A2);
}

void loop() {
	int i = getidx(y);
	if (tc > 30) {
		tc = 0;
		if (i == 0) cw();
		else if (i == 2) ccw();
	}
}
