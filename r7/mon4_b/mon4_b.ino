#define USE_TIMER3_ISR
#include "mono_con.h"

const int D2 = 18; // SW
const int D3 = 19; // TSW

int SW  = 0;
int TSW = 0;

int prev_SW  = 0;
int prev_TSW = 0;

int status = 0;
int tgl    = 1;

int count = 0;
int ten   = 0;
int one   = 0;

word in, bz;

ISR (TIMER3_COMPA_vect) {
  if (in > 5) {
    in = 0;

    SW  = digitalRead(D2);
    TSW = digitalRead(D3);
  }

  in++;

  if (TSW == HIGH) bz++;
}

void setup() {
  config_init();
  serial_init();

  disp(num[0], num[0]);
}

void loop() {
  if (TSW == HIGH) {
    // on

    if (TSW != prev_TSW) tgl = 1;

    if (bz > 1000) {
      bz = 0;

      if (tgl) {
        tone(BZ_PIN, 2000);
      } else {
        tone(BZ_PIN, 500);
      }

      if (++count > 99) count = 0;

      ten = count / 10;
      one = count % 10;

      disp(num[ten], num[one]);

      tgl = !tgl;
    }

  } else {
    // off

    bz = 1000;
    noTone(BZ_PIN);

    if (SW == LOW && prev_SW == HIGH) {
      count = 0;
      ten   = 0;
      one   = 0;

      disp(num[0], num[0]);
    }

  }

  prev_SW  = SW;
  prev_TSW = TSW;
}