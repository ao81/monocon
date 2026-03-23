#define USE_TIMER3_ISR
#include "mono_con.h"

const int D2 = 18; //タクトスイッチをデジタルD2へ接続。
const int D3 = 19; //トグルスイッチをデジタルD3へ接続。

word sTime;
word spm;
word wait;

int SW = 0;
int TSW = 0;

void setup(void) {
  config_init();
  serial_init();
}

ISR(TIMER3_COMPA_vect) {
  if (sTime > 5) {
    sTime = 0;

    SW = digitalRead(D2);
    TSW = digitalRead(D3);
  }
 
  sTime++;
  spm++;
  wait++;
}

int phase = 0;
int rotate = 0;
int status = 1;
int prevSW = HIGH;
int dir = 1;
const int speed = 15;

void loop(void) {
  if (TSW == HIGH) {
    dir = 1;
  } else {
    dir = -1;
  }

  if (SW == LOW && prevSW == HIGH && status == 0) {
    status = 1;
  }

  if (status == 1) {
    if (spm > speed && rotate < 120) {
      spm = 0;
      rotate++;
      lm.color.SM = stepm_init(phase);
      led_stepmotor(lm.b8);
      phase += dir;
      if (phase > 3) phase = 0;
      else if (phase < 0) phase = 3;
    }
    
    if (rotate >= 120) {
      status = 0;
      wait = 0;
      rotate = 0;
    }

  }

  prevSW = SW;
}