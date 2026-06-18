#define USE_TIMER3_ISR
#include "mono_con.h"

const int D2 = 18; //タクトスイッチをデジタルD2へ接続。
const int D3 = 19; //トグルスイッチをデジタルD3へ接続。

word sTime;
word spm;
word tgl;

int SW = 0;
int TSW = 0;

void setup(void) {
  config_init();
  serial_init();

  pinMode(D2, INPUT);//IN_D2
  pinMode(D3, INPUT);//IN_D3
}

ISR(TIMER3_COMPA_vect) {
  if (sTime > 5) {
    sTime = 0;

    SW = digitalRead(D2);
    TSW = digitalRead(D3);
  }
 
  sTime++;
  spm++;
}

int phase = 0;

void loop(void) {
  if(spm > 10 && SW == LOW) {
    if (TSW == HIGH) {
      spm = 0;
      lm.color.SM = stepm_init(phase);
      led_stepmotor(lm.b8);
      phase++;
      if (phase > 3) phase = 0;

    } else {
      spm = 3;
      lm.color.SM = stepm_init(phase);
      led_stepmotor(lm.b8);
      phase--;
      if (phase < 0) phase = 3;
      
    }
  }
}