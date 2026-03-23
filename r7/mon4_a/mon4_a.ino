<<<<<<< HEAD
#define USE_TIMER3_ISR
#include "mono_con.h"

void setup() {
  config_init();
  serial_init();
  noTone(BZ_PIN);
  disp(num[0], num[0]);
}

int bz_pitch[] = { 1000, 500 };
int bz_index = 0, cnt = 0;
int ten = 0, one = 0;
word tc;

ISR (TIMER3_COMPA_vect) {
  tc++;
}

void loop() {
  if (tc > 1000) {
    tc = 0;
    if (++bz_index > 1) bz_index = 0;
    
    if (++cnt > 99) cnt = 0;
    ten = cnt / 10;
    one = cnt % 10;

    disp(num[ten], num[one]);
  }
  tone(BZ_PIN, bz_pitch[bz_index]);
=======
#define USE_TIMER3_ISR
#include "mono_con.h"

void setup() {
  config_init();
  serial_init();
  noTone(BZ_PIN);
  disp(num[0], num[0]);
}

int bz_pitch[] = { 1000, 500 };
int bz_index = 0, cnt = 0;
int ten = 0, one = 0;
word tc;

ISR (TIMER3_COMPA_vect) {
  tc++;
}

void loop() {
  if (tc > 1000) {
    tc = 0;
    if (++bz_index > 1) bz_index = 0;
    
    if (++cnt > 99) cnt = 0;
    ten = cnt / 10;
    one = cnt % 10;

    disp(num[ten], num[one]);
  }
  tone(BZ_PIN, bz_pitch[bz_index]);
>>>>>>> 91b3e08 (r7/mon4_a/)
}