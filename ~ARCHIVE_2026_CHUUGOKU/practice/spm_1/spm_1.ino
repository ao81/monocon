#include "mono_con.h"

void setup() {
  config_init();
  serial_init();
}

int phase = 0;

void loop() {
  lm.bit.SM = stepm_init(phase);
  led_stepmotor(lm.b8);
  
  phase++;
  if (phase > 3) phase = 0;
  
  delay(10);
}
