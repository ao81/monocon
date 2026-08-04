//===================================================================================================================
//
//  高校生ものづくりコンテスト2026岡山県大会用 ArduinoShield
//
//    出力回路: 百エディケーション製
//    https://hyakuedu.wordpress.com/
//
//    CPU Board: Arduino MEGA 2560 R3
//                                     2026/04/15 OKAKO
//
//===================================================================================================================

#ifndef _MONO_CON_2026_H
#define _MONO_CON_2026_H

//--- Arduino ATmega2560 Input
#define pin_1	A1
#define pin_2	A2
#define pin_3	17  // D1
#define pin_4	18  // D2
#define pin_5	19  // D3
#define pin_6	40  // M1
#define pin_7	42  // M2
#define pin_8	20  // D4: GND

//--- OKAKO_Shield_CN2
#define BOARD_SW_PIN 26
#define BZ_PIN   11
#define FIN_PIN  6
#define RIN_PIN  7
#define LAT2_PIN 23
#define LAT1_PIN 22
#define SCK_PIN  24
#define SDI_PIN  25

//===================================================================================================================
//  ステッピングモータの励磁パターンを返す関数
//===================================================================================================================
int stepm_init(int n) {
	static const uint8_t tbl[5] = {
		0x9,  // A
		0xc,  // B
		0x6,  // nA
		0x3,  // nB
		0x00  // Stop
	};
	return (n >= 0 && n < 5) ? tbl[n] : 0;
}

//===================================================================================================================
//  フルカラーLEDとステッピングモータの動作制御クラス
//===================================================================================================================
class led_stepmotor {
public:
	uint8_t b8;
	inline operator uint8_t() const { return b8; }

	led_stepmotor() : b8(0) {}

	// フルカラーLED: 3bit GBR
	inline void led(uint8_t gbr) {
		b8 = (b8 & 0x0F) | ((gbr & 0x07) << 4);
	}

	// ステッピングモータ: 励磁フェーズ
	inline void spm(uint8_t phase) {
		b8 = (b8 & 0xF0) | ((uint8_t)stepm_init(phase) & 0x0F);
	}
	inline uint8_t sm() const { return b8 & 0x0F; }

	// 更新
	inline void update() const {
		digitalWrite(LAT2_PIN, LOW);
		shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, b8);
		digitalWrite(LAT2_PIN, HIGH);
	}
};
led_stepmotor lm;

//===================================================================================================================
//  レジスタ初期設定
//===================================================================================================================
void config_init(void) {
	// 入力
	pinMode(pin_1, INPUT);
	pinMode(pin_2, INPUT);
	pinMode(pin_3, INPUT);
	pinMode(pin_4, INPUT);
	pinMode(pin_5, INPUT);
	pinMode(pin_6, INPUT);
	pinMode(pin_7, INPUT);

	// D4=GND
	pinMode(pin_8, OUTPUT);
	digitalWrite(pin_8, LOW);

	// シールド側
	pinMode(BOARD_SW_PIN, INPUT);
	pinMode(BZ_PIN, OUTPUT);
	pinMode(RIN_PIN, OUTPUT);
	pinMode(FIN_PIN, OUTPUT);
	pinMode(LAT1_PIN, OUTPUT);
	pinMode(LAT2_PIN, OUTPUT);
	pinMode(SCK_PIN, OUTPUT);
	pinMode(SDI_PIN, OUTPUT);

	// 初期状態
	digitalWrite(LAT1_PIN, HIGH);
	digitalWrite(LAT2_PIN, HIGH);
	digitalWrite(SCK_PIN, LOW);
	digitalWrite(SDI_PIN, LOW);

	// Timer4: PWM 31.37kHz
	TCCR4B = (TCCR4B & 0b11111000) | 0x01;

#if defined(USE_TIMER3)
	// Timer3: 1ms割り込み
	TCCR3A = 0;
	TCCR3B = 0x0b;    // CTC, pre=64
	OCR3A = 249;      // 1kHz
	TIMSK3 = 0x02;    // OCIE3A
	sei();
#endif
}

//===================================================================================================================
//  シリアルデバイスの初期設定関数
//===================================================================================================================
void serial_init(void) {
	// U1, U2
	digitalWrite(LAT1_PIN, LOW);
	shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, 0x00);
	shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, 0x00);
	digitalWrite(LAT1_PIN, HIGH);

	// U3
	digitalWrite(LAT2_PIN, LOW);
	shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, 0x00);
	digitalWrite(LAT2_PIN, HIGH);
}

//===================================================================================================================
//  2桁7セグメントLEDの表示関数
//===================================================================================================================
void disp(char leftPat, char rightPat) {
	digitalWrite(LAT1_PIN, LOW);
	shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, (uint8_t)leftPat);
	shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, (uint8_t)rightPat);
	digitalWrite(LAT1_PIN, HIGH);
}

//===================================================================================================================
//  7セグメントLED 表示データ (0-9, blank)
//===================================================================================================================
const uint8_t num[11] = {
	0x3f, 0x06, 0x5b, 0x4f, 0x66,  // 0,1,2,3,4
	0x6d, 0x7d, 0x27, 0x7f, 0x6f,  // 5,6,7,8,9
	0x00                           // blank
};

#endif
