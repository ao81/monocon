#define USE_TIMER3_ISR
#include "mono_con.h"

const int D3 = 19; // TSW
int TSW = 0;

const int ledParam[] = { B001, B100, B010 };

int val = 1, ten_val = 0, one_val = 1;
bool status = false;

word in, tc;

ISR (TIMER3_COMPA_vect) {
  if (in > 5) {
    in = 0;

    TSW = digitalRead(D3);
  }

  in++;

  if (status) tc++;
}

void setup() {
  config_init();
  serial_init();

  disp(num[ten_val], num[one_val]);
}

void loop() {
  if (TSW == HIGH) {
    // on

    status = true;

    if (tc > 100) {
      tc = 0;

      if (++val > 99) val = 0;

      ten_val = val / 10;
      one_val = val % 10;

      disp(num[ten_val], num[one_val]);
    }

    if (one_val == 0) {
      lm.color.GBR = ledParam[ten_val % 3];
      led_stepmotor(lm.b8);
    }

  } else {
    // off

    status = false;
  }
}