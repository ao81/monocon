#include "mono_con.h"

const char p[] = { 0x01, 0x06, 0x08, 0x30 };
const char q[] = { 0x01, 0x30, 0x08, 0x06 };

void stepm(int n, int d) {
  for (int i = 0; i < n; i++) {
    lm.b8 = 0;
    lm.bit.SM = stepm_init(d > 0 ? i & 3 : 3 - (i & 3));
    led_stepmotor(lm.b8);
    delay(20);
  }
  led_stepmotor(0);
}

void col(int c) {
  lm.b8 = 0;
  lm.color.GBR = c;
  led_stepmotor(lm.b8);
}

void setup() {
  config_init();
  serial_init();
}

void loop() {
  while (digitalRead(_USER_CON_4PIN));
  while (!digitalRead(_USER_CON_4PIN));

  if (digitalRead(_USER_CON_5PIN)) {
    stepm(30, 1);
    analogWrite(FIN_PIN, 60);  delay(2000); analogWrite(FIN_PIN, 0);
    for (int i = 0; i < 4; i++) { disp(0, p[i]); delay(1000); }
    disp(0, 0);
    col(1); delay(1000);
    col(5); delay(1000);
    col(2); delay(1000);
    col(0);
    tone(BZ_PIN, 200, 2000); delay(2000);
  } else {
    for (int i = 0; i < 5; i++) { tone(BZ_PIN, 2000, 100); delay(200); }
    for (int i = 0; i < 6; i++) { col(7); delay(250); col(0); delay(250); }
    for (int i = 0; i < 4; i++) { disp(p[i], 0); delay(1000); }
    disp(0, 0);
    analogWrite(RIN_PIN, 200); delay(3000); analogWrite(RIN_PIN, 0);
    stepm(60, -1);
  }
}
