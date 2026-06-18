#include "mono_con.h"

void setup() {
  config_init();
  serial_init();
}

int phase = 3;

void loop() {
  lm.bit.SM = stepm_init(phase);
  led_stepmotor(lm.b8);
  
  phase--;
  if (phase < 0) phase = 3;
  
  delay(10);
}
