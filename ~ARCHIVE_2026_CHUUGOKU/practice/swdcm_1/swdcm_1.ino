#define USE_TIMER3_ISR
#include "mono_con.h"

//R08ものづくりコンテストシールドテスト
//入力回路はR07県大会の回路を使用
//GND:D4
//トグルスイッチ:D3
//タクトスイッチ:D2
//フォトインタラプタ:D1
//２軸ジョイスティック
//X軸,Y軸の読み取り:A1,A2

const int x_pin = A1; //X軸の出力をアナログA1へ接続。
const int y_pin = A2; //Y軸の出力をアナログA2へ接続。

const int D1 = 17; //フォトインタラプタをデジタルD1へ接続。
const int D2 = 18; //タクトスイッチをデジタルD2へ接続。
const int D3 = 19; //トグルスイッチをデジタルD3へ接続。
const int D4 = 20; //GNDに設定しD4へ接続。

const int M1 = 40; //M1（予備）へ接続。
const int M2 = 42; //M2（予備）へ接続。

int x_pos = 0; //初期状態設定。
int y_pos = 0;//初期状態設定。

int PHO = 0;
int SW = 0;
int TSW = 0;

float x_convert = 0.0f; //計算値の変数設定、オーバーフローしないようにflaot設定
float y_convert = 0.0f; //計算値の変数設定、オーバーフローしないようにflaot設定

word sTime;

ISR (TIMER3_COMPA_vect) {
  if (sTime > 5) {
    sTime = 0;

    // PHO = digitalRead(D1);
    SW = digitalRead(D2);
    TSW = digitalRead(D3);
  }

  sTime++;
}

void DMcw(int speed) {
  analogWrite(FIN_PIN, speed);
  digitalWrite(RIN_PIN, LOW);
}

void DMccw(int speed) {
  digitalWrite(FIN_PIN, LOW);
  analogWrite(RIN_PIN, speed);
}

void DMstop(void) {
  digitalWrite(FIN_PIN, HIGH);
  digitalWrite(RIN_PIN, HIGH);
}

void setup() {
  config_init();
  serial_init();
 
  pinMode(20, OUTPUT); //IN_D4
  digitalWrite(20, LOW); //IN_D4をGNDと設定

  pinMode(A1, INPUT);//IN_A1
  pinMode(A2, INPUT);//IN_A2
  pinMode(D1, INPUT);//IN_D1
  pinMode(D2, INPUT);//IN_D2
  pinMode(D3, INPUT);//IN_D3
  pinMode(M1, INPUT);//IN_M1
  pinMode(M2, INPUT);//IN_M2

}

void loop() {
  if (SW == LOW) {
    if (TSW == HIGH) {
      // TSW:ON SW:ON
      DMcw(100);
    } else {
      // TSW:ON SW:OFF
      DMccw(100);
    }
  } else {
    DMstop();
  }
}
