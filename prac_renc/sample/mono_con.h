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
#define pin1	A1
#define pin2	A2
#define pin3	17  // D1
#define pin4	18  // D2
#define pin5	19  // D3
#define pin6	40  // M1
#define pin7	42  // M2
#define pin8	20  // D4: GND

//--- OKAKO_Shield_CN2
#define BOARD_SW_PIN 26
#define BZ_PIN   11
#define FIN_PIN  6
#define RIN_PIN  7
#define LAT2_PIN 23
#define LAT1_PIN 22
#define SCK_PIN  24
#define SDI_PIN  25

//--- led_stepmotor::spm() の引数用
#define CW		0xFF
#define CCW		0xFE
#define STOP	0xFD

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
private:
	uint8_t b8 = 0;
	uint8_t cur_phase = 0;

public:
	// フルカラーLED: 3bit GBR
	led_stepmotor& led(uint8_t gbr) {
		b8 = (b8 & 0x0F) | ((gbr & 0x07) << 4);
		return *this;
	}

	// ステッピングモータ: 励磁フェーズ
	led_stepmotor& spm(uint8_t phase) {
		uint8_t p;
		if (phase == CW) {
			cur_phase = (cur_phase + 3) & 0x03;
			p = cur_phase;
		} else if (phase == CCW) {
			cur_phase = (cur_phase + 1) & 0x03;
			p = cur_phase;
		} else if (phase < 4) {
			cur_phase = phase;
			p = phase;
		} else {
			p = 4;
		}
		b8 = (b8 & 0xF0) | (stepm_init(p) & 0x0F);
		return *this;
	}

	// 更新
	void flush() const {
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
	pinMode(pin1, INPUT);
	pinMode(pin2, INPUT);
	pinMode(pin3, INPUT);
	pinMode(pin4, INPUT);
	pinMode(pin5, INPUT);
	pinMode(pin6, INPUT);
	pinMode(pin7, INPUT);

	// D4=GND
	pinMode(pin8, OUTPUT);
	digitalWrite(pin8, LOW);

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
	cli();
	TCCR3A = 0;
	TCCR3B = 0;
	TCNT3 = 0;
	OCR3A = F_CPU / 64 / 10000 - 1;   // 16MHz/64/1000-1 = 249
	TCCR3B |= (1 << WGM32);
	TCCR3B |= (1 << CS31) | (1 << CS30);
	TIMSK3 |= (1 << OCIE3A);
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
	shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, leftPat);
	shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, rightPat);
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
