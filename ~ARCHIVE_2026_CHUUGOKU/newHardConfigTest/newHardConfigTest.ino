///////////////////////////////////////////////////////////////////////////////
//
// hardTest --- 制御対象回路の動作テスト
//
// 7セグ（左桁）---デバイス番号（１～３）
//                タクトスイッチをダブルクリックするごとに１→２→３→１→２→…と変化
//                １：FullColor LED
//                ２：SteppingMotor
//                ３：DC Motor
// 7セグ（右桁）---動作番号（０～　）
//                タクトスイッチをクリックするごとに，各デバイスに応じて０→１→２→…とサイクリックに変化
//                数字に応じた動作
//
///////////////////////////////////////////////////////////////////////////////
#define USE_TIMER3_ISR
#include "mono_con.h"

boolean tsw = false;
boolean tswFlag = false;
boolean kFlag = false;

int kst;
int devNo = 1;
int value = 0;
int kCode;
int kTime;
int ex;
int toneTime = 0;

word tc, sTime;

///// 各デバイス動作を定義したデータ /////
int ledPara[] = { B000, B001, B010, B100, B011, B110, B101, B111 };  // FullColor LED 点灯データ
int smPara[] = { 10, 20, 40, -40, -20, -10 };                        // Stepping motor 回転データ（単位はms/step，+:CW, -:CCW）
int dmPara[] = { 0, 50, 100, 150, 0, -50, -100, -150 };              // DC motor 回転データ（1～255:1%～100%，0:停止，+:CW, -CCW）

///////////////////////////////////////////////////////////////////////////////
void setup(void) {
  config_init();
  serial_init();
  noTone(BZ_PIN);

  tsw = digitalRead(BOARD_SW_PIN);  // タイマ割込みでサンプリングン入力するので，必ず最初の値をsetup()内で読んでおく
  devNo = 1;
  value = 0;
  disp(num[devNo], num[value]);

  // Serial.begin(115200);
  // Serial.print("start");
}

///////////////////////////////////////////////////////////////////////////////
ISR(TIMER3_COMPA_vect) {
  ///// 入力デバイスのサンプリング入力 /////
  if (sTime > 5) {
    sTime = 0;
    tsw = digitalRead(BOARD_SW_PIN);
  }

  tc++;
  sTime++;

  ///// ダブルクリック判定処理用 /////
  if (kTime > 0) kTime--;

  ///// ブザー処理用 /////
  if (toneTime > 0) {
    toneTime--;
  } else {
    noTone(BZ_PIN);
  }
}

///////////////////////////////////////////////////////////////////////////////
int KeySense(void) {
  switch (kst) {
    case 0:
      if (tsw == LOW) {
        tswFlag = true;
        kTime = 500;
        kst = 1;
        return (-1);  // key in proccessing
      }
      return (0);  // no key in
      break;

    case 1:
      if (kTime > 0) {
        if (tsw == HIGH) {
          tswFlag = false;
        }
        if ((tsw == LOW) && (tswFlag == false)) {
          kst = 3;
          toneTime = 200;
          tone(BZ_PIN, 4000);
          return (2);  // double click!
        }
        return (-1);  // key in proccessing
      } else {
        kst = 3;
        toneTime = 200;
        tone(BZ_PIN, 1000);
        return (1);  // single click!
      }
      break;

    case 3:  // 入力確定後のタクトスイッチがoffを確認
      if (tsw == HIGH) {
        kst = 0;
      }
      return (0);
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////
void DMstop(void) {
  digitalWrite(FIN_PIN, HIGH);
  digitalWrite(RIN_PIN, HIGH);
}

///////////////////////////////////////////////////////////////////////////////
void DMcw(int speed) {
  analogWrite(FIN_PIN, speed);
  digitalWrite(RIN_PIN, LOW);
}

///////////////////////////////////////////////////////////////////////////////
void DMccw(int speed) {
  digitalWrite(FIN_PIN, LOW);
  analogWrite(RIN_PIN, speed);
}

///////////////////////////////////////////////////////////////////////////////
// main
///////////////////////////////////////////////////////////////////////////////
void loop(void) {
  kCode = KeySense();

  if (kCode == 2) {
    devNo++;
    if (devNo > 3) devNo = 1;
    value = 0;
    disp(num[devNo], num[value]);
  }

  switch (devNo) {
    case 1:  // FullColor LED
      if ((kCode == 1) && (kFlag == false)) {
        kFlag = true;
        value++;
        if (value > sizeof(ledPara) / sizeof(ledPara[0]) - 1) value = 0;
        disp(num[devNo], num[value]);
        lm.color.GBR = ledPara[value];
        led_stepmotor(lm.b8);
      }
      break;

    case 2:  // stepping motor
      if ((kCode == 1) && (kFlag == false)) {
        kFlag = true;
        value++;
        if (value > sizeof(smPara) / sizeof(smPara[0]) - 1) value = 0;
        disp(num[devNo], num[value]);
      }
      if (smPara[value] > 0) {
        if (tc > smPara[value]) {
          tc = 0;
          lm.bit.SM = stepm_init(ex);
          led_stepmotor(lm.b8);
          ex++;
          if (ex > 3) ex = 0;
        }
      } else {
        if (tc > -smPara[value]) {
          tc = 0;
          lm.bit.SM = stepm_init(ex);
          led_stepmotor(lm.b8);
          ex--;
          if (ex < 0) ex = 3;
        }
      }
      break;

    case 3:  // dc motor
      if ((kCode == 1) && (kFlag == false)) {
        kFlag = true;
        value++;
        if (value > sizeof(dmPara) / sizeof(dmPara[0]) - 1) value = 0;
        disp(num[devNo], num[value]);
      }
      if (dmPara[value] == 0) {
        DMstop();
      } else if (dmPara[value] > 0) {
        DMcw(dmPara[value]);
      } else {
        DMccw(-dmPara[value]);
      }
      break;

    default:
      break;
  }
  
  if ((kCode == 0) && (kFlag == true)) kFlag = false;
}