#include "mono_con.h"

void setup() {
  config_init();
  serial_init();
}

void DMcw(int speed) {
  analogWrite(FIN_PIN, speed);
  digitalWrite(RIN_PIN, LOW);
}

void loop() {
  DMcw(100);
}
