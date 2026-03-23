#define USE_TIMER3_ISR
#include "mono_con.h"

word sTime;
word spm;
word change;

void setup(void) {
  config_init();
  serial_init();
}

ISR(TIMER3_COMPA_vect) {
  if (sTime > 5) {
    sTime = 0;
  }
 
  sTime++;
  spm++;
  change++;
}

int phase = 0;
int dir = 1;
int speed[10] = { 100, 90, 80, 70, 60, 50, 40, 30, 20, 10 };
int spdidx = 0;

void loop(void) {
  if(spm > speed[spdidx] && spdidx != 10) {
    spm = 0;
    lm.color.SM = stepm_init(phase);
    led_stepmotor(lm.b8);
    phase++;
    if (phase > 3) phase = 0;
  }

  if (change > 5000) {
    change = 0;
    if (spdidx >= 10) dir = -1;
    if (spdidx <= 0) dir = 1;
    spdidx += dir;
  }
}