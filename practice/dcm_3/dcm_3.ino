#include "mono_con.h"

void setup() {
  config_init();
  serial_init();
}

void DMcw(int speed) {
  analogWrite(FIN_PIN, speed);
  digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed) {
  digitalWrite(FIN_PIN, LOW);
  analogWrite(RIN_PIN, speed);
}

void DMstop(void) {
  digitalWrite(FIN_PIN, HIGH);
  digitalWrite(RIN_PIN, HIGH);
}

void loop() {
  DMcw(100);
  delay(5000);
  DMstop();
  delay(5000);
  DMccw(100);
  delay(5000);
  DMstop();
  delay(5000);
}
