#include "mono_con.h"

void setup() {
  config_init();
  serial_init();
}

int ledPara[] = { B000, B001, B010, B100, B011, B110, B101, B111 };
int value = 0, dir = 1;

void loop() {
  lm.color.GBR = ledPara[value];
  led_stepmotor(lm.b8);
  delay(1000);

  value += dir;

  if (value <= 0 || value >= 7) {
    dir = -dir;
  }
}
