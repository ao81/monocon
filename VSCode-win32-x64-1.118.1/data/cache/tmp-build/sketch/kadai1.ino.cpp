#include <Arduino.h>
#line 1 "C:\\Users\\ao\\Desktop\\monokon_release\\sample\\kadai1\\kadai1.ino"
#define USE_TIMER3_ISR
#include "mono_con.h"

word in;

#line 12 "C:\\Users\\ao\\Desktop\\monokon_release\\sample\\kadai1\\kadai1.ino"
void setup();
#line 17 "C:\\Users\\ao\\Desktop\\monokon_release\\sample\\kadai1\\kadai1.ino"
void loop();
#line 6 "C:\\Users\\ao\\Desktop\\monokon_release\\sample\\kadai1\\kadai1.ino"
ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	
}
