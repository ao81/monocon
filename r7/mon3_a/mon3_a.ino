#define USE_TIMER3_ISR
#include "mono_con.h"

const int y_pin = A1;

int y_pos = 0;

word in;

void setup() {
  config_init();
  serial_init();

  pinMode(A2, INPUT);
}

ISR (TIMER3_COMPA_vect) {
  if (in > 5) {
    in = 0;
    y_pos = analogRead(y_pin);
  }
  in++;
}

void DMcw(int spd) {
  analogWrite(FIN_PIN, spd);
  digitalWrite(RIN_PIN, LOW);
}

void DMccw(int spd) {
  digitalWrite(FIN_PIN, LOW);
  analogWrite(RIN_PIN, spd);
}

void DMstop() {
  digitalWrite(FIN_PIN, HIGH);
  digitalWrite(RIN_PIN, HIGH);
}

void loop() {
  if (y_pos < 320) {
    DMccw(100);
  } else if (y_pos > 780) {
    DMcw(100);
  } else {
    DMstop();
  }
}