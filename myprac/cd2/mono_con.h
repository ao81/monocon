//===================================================================================================================
//
//  高校生ものづくりコンテスト2026岡山県大会用 ArduinoShield  (最適化・高速化版 / API完全互換)
//
//    出力回路: 百エディケーション製
//    https://hyakuedu.wordpress.com/
//
//    CPU Board: Arduino MEGA 2560 R3
//                                     2026/01/08 OKAKO
//                                     最適化: 直接ポート操作による shiftOut 高速化
//
//  -- 互換性 --
//    元の API (関数名・引数・変数名) を完全に維持。既存の .ino をそのまま使用可能。
//
//  -- 最適化点 --
//    [1] shiftOut() を直接ポート操作版に置換  …… 約 30倍高速 (約 100us → 約 3us / byte)
//    [2] LAT/SCK/SDI 制御を _BV() マクロ化     …… 2クロック (sbi/cbi 命令) で操作
//    [3] ビットフィールドの型を int → uint8_t  …… 構造体サイズを半減 (16bit → 8bit)
//    [4] union の b8 を int → uint8_t          …… 隠れたバグ修正 (16bit書込で上位が破壊)
//    [5] 関数を inline 化                      …… 関数呼び出しオーバヘッド削減
//    [6] stepm_init の配列に static const      …… 呼び出しごとの再初期化を回避
//    [7] stepm_init に範囲チェック追加          …… 配列範囲外アクセス防止
//
//===================================================================================================================
#ifndef _MONO_CON_2026_H
#define _MONO_CON_2026_H

#include <Arduino.h>
#include <stdint.h>

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

//-------------------------------------------------------------------
// 直接ポート操作マクロ (★高速化の核)
//   LAT1/LAT2/SCK/SDI は全て PORTA に集約 (PA0〜PA3) されているため、
//   sbi/cbi 命令 (2クロック) で操作可能。digitalWrite (~80クロック) より大幅に高速。
//
//   ピン番号 → ポートビット対応:
//     LAT1_PIN(22) = PA0    LAT2_PIN(23) = PA1
//     SCK_PIN (24) = PA2    SDI_PIN (25) = PA3
//-------------------------------------------------------------------
#define LAT1_BIT  PA0
#define LAT2_BIT  PA1
#define SCK_BIT   PA2
#define SDI_BIT   PA3

#define LAT1_HIGH()  (PORTA |=  _BV(LAT1_BIT))
#define LAT1_LOW()   (PORTA &= ~_BV(LAT1_BIT))
#define LAT2_HIGH()  (PORTA |=  _BV(LAT2_BIT))
#define LAT2_LOW()   (PORTA &= ~_BV(LAT2_BIT))
#define SCK_HIGH()   (PORTA |=  _BV(SCK_BIT))
#define SCK_LOW()    (PORTA &= ~_BV(SCK_BIT))
#define SDI_HIGH()   (PORTA |=  _BV(SDI_BIT))
#define SDI_LOW()    (PORTA &= ~_BV(SDI_BIT))

//-------------------------------------------------------------------
// 高速 shiftOut  (MSBFIRST, PORTA 上の SDI/SCK 専用)
//   標準 shiftOut() : 約 100us / byte
//   この実装       : 約   3us / byte  (★ 約 30倍高速)
//-------------------------------------------------------------------
__attribute__((always_inline))
static inline void fast_shift_out(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    if (data & 0x80) SDI_HIGH();
    else             SDI_LOW();
    SCK_HIGH();
    data <<= 1;
    SCK_LOW();
  }
}

//--- セグメントLED 0-9,blank
const uint8_t num[11] = { 0x3f, 0x06, 0x5b, 0x4f, 0x66,  //0, 1, 2, 3, 4
                          0x6d, 0x7d, 0x27, 0x7f, 0x6f,  //5, 6, 7, 8, 9
                          0x00 };                         //blank

//--- レジスタ初期設定
inline void config_init(void) {
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
  //   16MHz / 64 / 1000Hz - 1 = 249  → 1kHz (1ms)
  TCCR3A = 0;
  TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);    // CTC, 分周64
  OCR3A = (F_CPU / 64UL / 1000UL) - 1;                  // = 249
  TIMSK3 = (1 << OCIE3A);                                // Compare-A 割込み有効
#endif
}

//--- 構造体宣言bitset (★ uint8_t 化で省メモリ)
struct bitset {
  uint8_t SDI : 1;   // bit0 シリアルデータ
  uint8_t SCK : 1;   // bit1 シリアルクロック
  uint8_t LAT1 : 1;   // bit2 7seg LED 系ラッチ
  uint8_t LAT2 : 1;   // bit3 フルカラーLED,step モータ系ラッチ
  uint8_t DCM : 2;   // bit4,5 DC モータの動作モード
  uint8_t BZ : 1;   // bit6 圧電スピーカ
  uint8_t TSW : 1;   // bit7 タクトスイッチ入力
};
struct bitset RC;     // 構造体変数

//--- 共用体宣言lm (LED & step Motor)  (★ uint8_t 化、b8のバグ修正)
union {
  struct {
    uint8_t SM : 4;   // bit0-3 ステッピングモータ励磁信号
    uint8_t R : 1;   // bit4 フルカラーLED 赤色
    uint8_t B : 1;   // bit5 フルカラーLED 青色
    uint8_t G : 1;   // bit6 フルカラーLED 緑色
    uint8_t res : 1;   // bit7 未使用
  } bit;               // bit アクセス名
  struct {
    uint8_t SM : 4;
    uint8_t GBR : 3;   // フルカラーLED, カラーコードを3bitで指定
    uint8_t res : 1;
  } color;             // GBRを3ビットアクセス名
  uint8_t b8;          // byte アクセス名
} lm;                  // 共用体変数名

//--- U1,U2,U3初期化
inline void serial_init(void) {
  // U1, U2
  LAT1_LOW();
  fast_shift_out(0x00);
  fast_shift_out(0x00);
  LAT1_HIGH();

  // U3
  LAT2_LOW();
  fast_shift_out(0x00);
  LAT2_HIGH();
}

//--- 2桁7セグメントLEDの表示関数  (★ 約30倍高速化)
inline void disp(char leftPat, char rightPat) {
  LAT1_LOW();
  fast_shift_out((uint8_t)leftPat);
  fast_shift_out((uint8_t)rightPat);
  LAT1_HIGH();
}

//--- ステッピングモータの初期設定関数  (★ static const化 + 範囲チェック)
inline int stepm_init(int n) {
  static const int spm[5] = { 0x9,     // A
                              0xc,     // B
                              0x6,     // nA
                              0x3,     // nB
                              0x00 };  // Stop
  if ((unsigned)n > 4) n = 4;          // 範囲外は Stop
  return spm[n];
}

//--- フルカラーLEDとステッピングモータの動作制御関数
inline void led_stepmotor(char n) {
  LAT2_LOW();
  fast_shift_out((uint8_t)n);
  LAT2_HIGH();
}

#endif