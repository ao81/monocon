//===================================================================================================================
//
//  高校生ものづくりコンテスト2026岡山県大会用 ArduinoShield
//
//    出力回路: 百エディケーション製
//    https://hyakuedu.wordpress.com/
//
//    CPU Board: Arduino MEGA 2560 R3
//                                     2026/01/08 OKAKO
//
//===================================================================================================================
#ifndef _MONO_CON_2026_H
#define _MONO_CON_2026_H

//--- Arduino  ATmega2560 Input
#define _USER_CON_1PIN A1
#define _USER_CON_2PIN A2
#define _USER_CON_3PIN 17
#define _USER_CON_4PIN 18
#define _USER_CON_5PIN 19
#define _USER_CON_6PIN 40
#define _USER_CON_7PIN 42
#define _USER_CON_8PIN 20 //GND

//--- OKAKO_Shield_CN2
#define BOARD_SW_PIN 26
#define BZ_PIN 11
#define FIN_PIN 6
#define RIN_PIN 7
#define LAT2_PIN 23
#define LAT1_PIN 22
#define SCK_PIN 24
#define SDI_PIN 25

//--- セグメントLED 0-9,blank
int num[11] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66,  //0, 1, 2, 3, 4
                0x6d, 0x7d, 0x27, 0x7f, 0x6f,  //5, 6, 7, 8, 9
                0x00 };                        //blank

//--- レジスタ初期設定
void config_init(void) {
  pinMode(_USER_CON_1PIN, INPUT);
  pinMode(_USER_CON_2PIN, INPUT);
  pinMode(_USER_CON_3PIN, INPUT);
  pinMode(_USER_CON_4PIN, INPUT);
  pinMode(_USER_CON_5PIN, INPUT);
  pinMode(_USER_CON_6PIN, INPUT);
  pinMode(_USER_CON_7PIN, INPUT);

  pinMode(_USER_CON_8PIN, OUTPUT);
  digitalWrite(_USER_CON_8PIN, LOW); //IN_D4をGND設定 

  pinMode(BOARD_SW_PIN, INPUT);
  pinMode(BZ_PIN, OUTPUT);
  pinMode(RIN_PIN, OUTPUT);
  pinMode(FIN_PIN, OUTPUT);
  pinMode(LAT1_PIN, OUTPUT);
  pinMode(LAT2_PIN, OUTPUT);
  pinMode(SCK_PIN, OUTPUT);
  pinMode(SDI_PIN, OUTPUT);

  ////////// DCモータ制御用PWM信号生成用のタイマ(Timer4)の周波数を31.37KHzに変更 //////////
  TCCR4B = (TCCR4B & 0b11111000) | 0x01;

#ifdef USE_TIMER3_ISR
  ////////// 1ms割り込み用のタイマ(Timer3)の初期化 //////////
  TCCR3A = 0;
  TCCR3B = 0x0b;
  // OCR3A = 124;
  OCR3A = 249;
  TIMSK3 = 2;
#endif
}

//--- 構造体宣言bitset
struct bitset {
  int SDI : 1;   // bit0 シリアルデータ
  int SCK : 1;   // bit1 シリアルクロック
  int LAT1 : 1;  // bit2 7seg LED 系ラッチ
  int LAT2 : 1;  // bit3 フルカラーLED,step モータ系ラッチ
  int DCM : 2;   // bit4,5 DC モータの動作モード
  int BZ : 1;    // bit6 圧電スピーカ
  int TSW : 1;   // bit7 タクトスイッチ入力
};
struct bitset RC;  // 構造体変数

//--- 共用体宣言lm (LED & step Motor)
union {
  struct
  {               //--- 構造体宣言，bit というグループ名
    int SM : 4;   // bit0 ～ bit3 ステッピングモータ励磁信号
    int R : 1;    // bit4 フルカラーLED，赤色
    int B : 1;    // bit5 フルカラーLED，青色
    int G : 1;    // bit6 フルカラーLED，緑色
    int res : 1;  // bit7 未使用
  } bit;          // bit アクセス名
  struct
  {
    int SM : 4;
    int GBR : 3;  // フルカラーLED, カラーコードを3bitで指定
    int res : 1;
  } color;  // GBRを3ビットアクセス名
  int b8;   // byte アクセス名
} lm;       // 共用体変数名

//--- U1,U2,U3初期化
void serial_init(void) {
  //U1
  digitalWrite(LAT1_PIN, LOW);
  shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, 0x00);
  digitalWrite(LAT1_PIN, HIGH);

  //U2
  digitalWrite(LAT1_PIN, LOW);
  shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, 0x00);
  digitalWrite(LAT1_PIN, HIGH);

  //u3
  digitalWrite(LAT2_PIN, LOW);
  shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, 0x00);
  digitalWrite(LAT2_PIN, HIGH);
}

//--- 2桁7セグメントLEDの表示関数
int disp(char leftPat, char rightPat) {
  digitalWrite(LAT1_PIN, LOW);
  shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, leftPat);
  digitalWrite(LAT1_PIN, HIGH);

  digitalWrite(LAT1_PIN, LOW);
  shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, rightPat);
  digitalWrite(LAT1_PIN, HIGH);
}

//--- ステッピングモータの初期設定関数
int stepm_init(int n) {
  int spm[5] = { 0x9,     // A
                 0xc,     // B
                 0x6,     // nA
                 0x3,     // nB
                 0x00 };  // Stop

  return spm[n];
}

//--- フルカラーLEDとステッピングモータの動作制御関数
int led_stepmotor(char n) {
  digitalWrite(LAT2_PIN, LOW);
  shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, n);
  digitalWrite(LAT2_PIN, HIGH);
}

#endif
