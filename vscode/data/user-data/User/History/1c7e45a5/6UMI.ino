#define _USER_CON_4PIN 18
#define _USER_CON_8PIN 20 //GND

#define SCK_PIN 24
#define SDI_PIN 25

#define USE_TIMER3_ISR

void config_init(void) {
	pinMode(_USER_CON_4PIN, INPUT);

	pinMode(_USER_CON_8PIN, OUTPUT);
	digitalWrite(_USER_CON_8PIN, LOW); //IN_D4をGND設定 

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

//--- U1,U2,U3初期化
void serial_init(void) {
	// U1, U2
	
}

union {
	struct {               //--- 構造体宣言，bit というグループ名
		unsigned int SM : 4;   // bit0 ～ bit3 ステッピングモータ励磁信号
		unsigned int R : 1;    // bit4 フルカラーLED，赤色
		unsigned int B : 1;    // bit5 フルカラーLED，青色
		unsigned int G : 1;    // bit6 フルカラーLED，緑色
		unsigned int res : 1;  // bit7 未使用
	} bit;          // bit アクセス名
	struct {
		unsigned int SM : 4;
		unsigned int GBR : 3;  // フルカラーLED, カラーコードを3bitで指定
		unsigned int res : 1;
	} color;  // GBRを3ビットアクセス名
	int b8;   // byte アクセス名
} lm;       // 共用体変数名

void led_stepmotor(char n) {
	digitalWrite(LAT2_PIN, LOW);
	shiftOut(SDI_PIN, SCK_PIN, MSBFIRST, n);
	digitalWrite(LAT2_PIN, HIGH);
}

int sw, in = 0;

ISR (TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;
		sw = digitalRead(_USER_CON_4PIN);
	}
}

void setup() {
	config_init();
	serial_init();
	sw = digitalRead(_USER_CON_4PIN);
}

void loop() {
	if (sw == LOW) lm.color.GBR = B011;
	else lm.color.GBR = B000;
	led_stepmotor(lm.b8);
}