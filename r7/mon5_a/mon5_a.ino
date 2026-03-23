#define USE_TIMER3_ISR
#include "mono_con.h"

const int x_pin = A1;
const int D1 = 17;  // PH
const int D2 = 18;  // SW
const int D3 = 19;  // TSW

const int ledParam[2] = { B100, B000 };                                              // green, blank
const int hand[3] = { 0x3f, 0x3e, 0x73 };                                            // グー, チョキ, パー
const int char_cnt[3] = { 3, 6, 6 };                                                 // グリコゲームの文字数
const int pitch_hz[11] = { 254, 262, 277, 293, 311, 329, 349, 391, 440, 493, 523 };  // 音程ごとの周波数

int x_pos = 0;
int PH = 0;
int SW = HIGH;
int TSW = 0;

int prev_sw = HIGH, prev_tsw = HIGH;
int cp_hand = 0, pl_hand = 0;
int ex = 0;

int step_rem = 0;
int step_dir = 1;

word in, spm, ff, se;
int cnt = -1;

int playing_ff = 2;  // 0:pl 1:cp 2:stop
int playing_se = 2;  // 0:グリコ 1:チョコレート・パイナップル 2:stop
bool game_end = false;
bool need_check = false;

void setup() {
  config_init();
  serial_init();
  noTone(BZ_PIN);

  disp(num[10], num[10]);

  pinMode(A1, INPUT);  // IN_A1
  pinMode(D1, INPUT);  // IN_D1
  pinMode(D2, INPUT);  // IN_D2
  pinMode(D3, INPUT);  // IN_D3
}

ISR(TIMER3_COMPA_vect) {
  if (in > 5) {
    in = 0;

    x_pos = analogRead(A1);
    PH = digitalRead(D1);
    SW = digitalRead(D2);
    TSW = digitalRead(D3);
  }
  in++;

  if (spm > 400) {
    spm = 0;
    if (step_rem > 0) {
      lm.bit.SM = stepm_init(((ex % 4) + 4) % 4);
      led_stepmotor(lm.b8);
      ex += step_dir;
      step_rem--;
    }
  }
  spm++;

  if (cnt > 0) cnt--;
  if (SW == LOW && prev_sw == HIGH && !game_end) cnt = 3000;
  prev_sw = SW;

  if (playing_ff == 0) {  // pl
    if (ff <= 2800) ff++;
  } else if (playing_ff == 1) {  // cp
    if (ff <= 800) ff++;
  }
}

int input_hand() {
  if (x_pos < 225) {
    return 2;
  } else if (x_pos > 775) {
    return 0;
  } else {
    return 1;
  }
}

int judge(int p, int c) {  // じゃんけんの勝敗
  return (p - c + 3) % 3;
}

void step_n(int n) {  // +:cw -:ccw
  step_dir = (n > 0) ? 1 : -1;
  step_rem = abs(n);
}

void pl_fanfare() {
  playing_ff = 0;

  if (ff < 400) {
    tone(BZ_PIN, pitch_hz[1]);
  } else if (ff < 600) {
    tone(BZ_PIN, pitch_hz[5]);
  } else if (ff < 800) {
    tone(BZ_PIN, pitch_hz[7]);
  } else if (ff < 1600) {
    tone(BZ_PIN, pitch_hz[10]);
  } else if (ff < 2400) {
    tone(BZ_PIN, pitch_hz[9]);
  } else if (ff < 2800) {
    tone(BZ_PIN, pitch_hz[10]);
  } else {
    noTone(BZ_PIN);
    playing_ff = 2;
  }
}

void cp_fanfare() {
  playing_ff = 1;

  int phase = (ff / 200) % 4;
  int step = (ff % 200) / 50;

  if (ff < 800) {
    tone(BZ_PIN, pitch_hz[1 + step]);
  } else {
    noTone(BZ_PIN);
    playing_ff = 2;
  }
}



void loop() {
  if (playing_ff == 0) pl_fanfare();
  else if (playing_ff == 1) cp_fanfare();

  /* ゲームの勝敗判定 */
  if (need_check && step_rem == 0) {
    need_check = false;
    ff = 0;
    if (ex <= -14) {
      pl_fanfare();
      game_end = true;
    } else if (ex >= 14) {
      cp_fanfare();
      game_end = true;
    }
  }

  if (TSW == LOW) {
    lm.color.GBR = ledParam[0];
    if (cnt == 3000) disp(num[2], num[10]);
    if (cnt == 2000) disp(num[1], num[10]);
    if (cnt == 1000) disp(num[0], num[10]);
    if (cnt == 0) {
      cnt = -1;

      pl_hand = input_hand();
      do {  // あいこにならないため
        cp_hand = random(0, 3);
      } while (pl_hand == cp_hand);

      int result = judge(pl_hand, cp_hand);
      if (result == 1) {  // cpの勝ち
        disp(hand[pl_hand], hand[cp_hand] | 0x80);
        step_n(char_cnt[cp_hand]);
      } else if (result == 2) {  // playerの勝ち
        disp(hand[pl_hand] | 0x80, hand[cp_hand]);
        step_n(-char_cnt[pl_hand]);
      }

      if (result == 1 || result == 2) {
        need_check = true;
      }
    }

  } else {
    if (prev_tsw != TSW) {
      game_end = false;
      ex = 0;

      disp(num[10], num[10]);
      lm.color.GBR = ledParam[1];
    }
  }

  prev_tsw = TSW;
  led_stepmotor(lm.b8);
}