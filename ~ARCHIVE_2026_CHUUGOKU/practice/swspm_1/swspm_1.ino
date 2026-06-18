#define USE_TIMER3_ISR
#include "mono_con.h"

const int D2 = 18;
int SW = 0;

word sTime, tc;

ISR (TIMER3_COMPA_vect) {
  if (sTime > 5) {
    sTime = 0;

    SW = digitalRead(D2);
  }

  sTime++;
  tc++;
}

void setup() {
  config_init();
  serial_init();
}

int phase = 0,  status = 1;
int prevSW = HIGH;

void loop() {
  if (SW == LOW && prevSW == HIGH) {
    status = !status;
  }

  if (tc > 10 && status) {
    tc = 0;

    lm.bit.SM = stepm_init(phase);
    led_stepmotor(lm.b8);
    
    phase++;
    if (phase > 3) phase = 0;
  }

  prevSW = SW;
}
