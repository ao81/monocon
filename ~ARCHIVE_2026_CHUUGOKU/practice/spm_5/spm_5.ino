#define USE_TIMER3_ISR
#include "mono_con.h"

word sTime;
word spm;
word wait;

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
  wait++;
}

int phase = 0;
int rotate = 0;
int status = 1;
const int speed = 15;

void loop(void) {
  if (status == 1) {
    if (spm > speed && rotate < 120) {
      spm = 0;
      rotate++;
      lm.color.SM = stepm_init(phase);
      led_stepmotor(lm.b8);
      phase++;
      if (phase > 3) phase = 0;
    }
    
    if (rotate >= 120) {
      status = 0;
      wait = 0;
      rotate = 0;
    }

  } else if (status == 0) {
    if (wait >= 5000) {
      status = 1;
      phase = 0;
    }
  }
}