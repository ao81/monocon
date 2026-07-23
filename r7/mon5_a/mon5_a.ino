#define USE_TIMER3_ISR
#include "mono_con.h"

const int x_pin = A1;
const int D1 = 17;  // PH
const int D2 = 18;  // SW
const int D3 = 19;  // TSW

const int ledParam[2] = { B100, B000 };                                              // green, blank
const int handLed[3] = { 0x3f, 0x3e, 0x73 };                                            // グー, チョキ, パーのled表示
const int char_cnt[3] = { 3, 6, 6 };                                                 // グリコゲームの文字数
const int pitch_hz[11] = { 254, 262, 277, 293, 311, 329, 349, 391, 440, 493, 523 };  // 音程ごとの周波数
const int se_pitch_idx[3][6] = {
  { 6, 0, 1, -1, -1, -1 },
  { 1, 5, 7, 10, 0, 1 },
  { 5, 3, 1, 8, 0, 1 }
};

int x_pos = 0;
int PH = 0;
int SW = HIGH;
int TSW = 0;

int prev_sw = HIGH, prev_tsw = HIGH;
int ex = 0;

int step_rem = 0;
int step_dir = 1;

word in, spm, ff, se;
int cnt = -1;

int playing_ff = 2;  // 0:pl 1:cp 2:stop
int se_index = 0;   // 0~5

bool result_check = false;

typedef enum {
  PLAYING,
  WAITING,
  GAMEEND
} GameState;
GameState gs = GAMEEND;

typedef enum {
  GU,
  TYO,
  PA,
  NONE
} Hand;
Hand cp_hand = NONE, pl_hand = NONE;
Hand se_hand = NONE;

void setup() {
  config_init();
  serial_init();
  noTone(BZ_PIN);

  disp(num[10], num[10]);
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

      se = 400;
    }
  }
  spm++;

  if (se > 0) {
    se--;
    if (se == 399) {
      if (se_index < char_cnt[se_hand]) {
        tone(BZ_PIN, pitch_hz[se_pitch_idx[se_hand][se_index++]]);
      }
    } else if (se == 0) {
      noTone(BZ_PIN);
    }
  }

  if (cnt > 0) cnt--;
  if (SW == LOW && prev_sw == HIGH && gs == PLAYING) {
    cnt = 3000; // じゃんけんのカウントダウン
    gs = WAITING;
  }
  prev_sw = SW;

  if (playing_ff == 0) {  // pl
    if (ff <= 2800) ff++;
  } else if (playing_ff == 1) {  // cp
    if (ff <= 800) ff++;
  }
}

Hand input_hand() {
  if (x_pos < 225) {
    return PA;
  } else if (x_pos > 775) {
    return GU;
  } else {
    return TYO;
  }
}

int judge(int p, int c) {  // じゃんけんの勝敗
  return (p - c + 3) % 3;
}

void step_n(int n, Hand winner) {  // +:cw -:ccw
  step_dir = (n > 0) ? 1 : -1;
  step_rem = abs(n);
  se_index = 0;
  se_hand = winner;
}

void fanfare(int type) {
  playing_ff = type;

  if (type == 0) { // pl
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

  } else if (type == 1) { // cp
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
}

void loop() {
  if (playing_ff == 0) fanfare(0);
  else if (playing_ff == 1) fanfare(1);

  /* ゲームの勝敗判定 */
  if (result_check && step_rem == 0) {
    result_check = false;
    gs = PLAYING;
    ff = 0;
    if (ex <= -14) {
      fanfare(0);
      gs = GAMEEND;
    } else if (ex >= 14) {
      fanfare(1);
      gs = GAMEEND;
    }
  }

  if (TSW == LOW) {
    if (prev_tsw != TSW) gs = PLAYING;

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
        disp(handLed[pl_hand], handLed[cp_hand] | 0x80);
        step_n(char_cnt[cp_hand], cp_hand);
      } else if (result == 2) {  // playerの勝ち
        disp(handLed[pl_hand] | 0x80, handLed[cp_hand]);
        step_n(-char_cnt[pl_hand], pl_hand);
      }

      result_check = true;
    }

  } else {
    if (prev_tsw != TSW) {
      gs = GAMEEND;
      ex = 0;

      noTone(BZ_PIN);
      disp(num[10], num[10]);
      lm.color.GBR = ledParam[1];
    }
  }

  prev_tsw = TSW;
  led_stepmotor(lm.b8);
}