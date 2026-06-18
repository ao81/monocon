#include "mono_con.h"

void setup() {
  config_init();
  serial_init();
}

void DMccw(int speed) {
  digitalWrite(FIN_PIN, LOW);
  analogWrite(RIN_PIN, speed);
}

void loop() {
  DMccw(100);
}
