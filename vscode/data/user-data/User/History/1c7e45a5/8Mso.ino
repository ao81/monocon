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