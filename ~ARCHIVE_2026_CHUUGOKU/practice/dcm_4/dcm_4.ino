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

int dmPara[12] = { 0, 50, 70, 90, 110, 130, 150, 170, 190, 210, 230, 0 };
int index = 0;
int dir = 1;

void loop() {
  if (dir == 1) {
    DMcw(dmPara[index]);
  } else {
    DMccw(dmPara[index]);
  }

  index += dir;
  if (index == 0 || index == 11) {
    dir = -dir;
  }

  delay(1000);
}
