#define USE_TIMER3_ISR
#include "mono_con.h"

const int D3 = 19;

int TSW = 0;
int ledBlank = B000;
int ledParam[] = { B001, B100, B010 };
int ledValue = 0;
bool prev_on = false;

word in, led;

ISR (TIMER3_COMPA_vect) {
  if (in > 5) {
    in = 0;
    TSW = digitalRead(D3);
  }
  in++;
  led++;
}

void setup() {
  config_init();
  serial_init();
}

void loop() {
  if (TSW == HIGH) {
    prev_on = true;
    if (led > 1000) {
      led = 0;
      lm.color.GBR = ledParam[ledValue];
      if (++ledValue > 2) ledValue = 0;
    }
  } else {
    if (prev_on) {
      if (--ledValue < 0) ledValue = 2;
      prev_on = false;
    }
    led = 1000;
    lm.color.GBR = ledBlank;
  }
  led_stepmotor(lm.b8);
}