#define USE_TIMER3_ISR
#include "mono_con.h"

const int D1 = 17; // PH
const int D2 = 18; // SW

int PH = 0;
int SW = 0;

word in;

ISR (TIMER3_COMPA_vect) {
  if (in > 5) {
    in = 0;
    PH = digitalRead(D1);
    SW = digitalRead(D2);
  }
  in++;
}

void setup() {
  config_init();
  serial_init();
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
  if (SW == LOW) {
    DMcw(100);
  } else if (PH == HIGH) {
    DMccw(100);
  } else {
    DMstop();
  }
}