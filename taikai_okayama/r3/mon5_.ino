// ============================================================
//  高校生ものづくりコンテスト2026岡山県大会
//  エンジェルナンバー算出プログラム（完全版）
//
//  CPU Board : Arduino MEGA 2560 R3
//  Shield    : 百エディケーション製 出力回路
//
// ---- ピン割り当て（mono_con.h より）----
//  ロータリーエンコーダ A相 : _USER_CON_6PIN (pin40)
//  ロータリーエンコーダ B相 : _USER_CON_7PIN (pin42)
//  SW1 (確定 / 次ステップ)  : _USER_CON_5PIN (pin19)
//  SW2 (リセット)           : _USER_CON_3PIN (pin17)
//  TSW (トグルSW 平成/令和) : _USER_CON_4PIN (pin18)
//
// ---- 状態とLED色 ----
//  IN_YEAR   : 赤  (GBR=B001)
//  IN_MONTH  : 緑  (GBR=B010)
//  IN_DAY    : 青  (GBR=B100)
//  OUT_RESULT: 白  (GBR=B111)
//
// ---- 和暦→西暦変換 ----
//  平成 n 年 = 1988 + n
//  令和 n 年 = 2018 + n
// ============================================================

#define USE_TIMER3_ISR
#include "mono_con.h"

// ============================================================
//  定数
// ============================================================

// フルカラーLED色 (GBRビット順)
//   [0]=IN_YEAR(赤), [1]=IN_MONTH(緑), [2]=IN_DAY(青), [3]=OUT_RESULT(白)
const int LED_COLOR[4] = { B001, B010, B100, B111 };

// 和暦オフセット
#define HEISEI_OFFSET  1988   // 平成 n 年 = 1988 + n
#define REIWA_OFFSET   2018   // 令和 n 年 = 2018 + n

// 平成・令和の入力範囲
#define HEISEI_MIN  1
#define HEISEI_MAX  31
#define REIWA_MIN   1
#define REIWA_MAX   6         // 余裕を持たせた上限（仕様は3）

// ============================================================
//  状態定義
// ============================================================
typedef enum {
	IN_YEAR,
	IN_MONTH,
	IN_DAY,
	OUT_RESULT,
} Status;

// ============================================================
//  グローバル変数
// ============================================================

// ---- タイマ割り込み用カウンタ ----
volatile word irqCount = 0;

// ---- スイッチ生値（ISR 内で更新） ----
volatile int tswRaw = HIGH;   // トグルSW現在値

// ---- チャタリング除去済みフラグ（ISR → loop） ----
volatile bool sw1Pressed = false;  // SW1 確定（立下りエッジ）
volatile bool sw2Pressed = false;  // SW2 リセット（立下りエッジ）
volatile bool tswChanged = false;  // トグルSW 変化検出
volatile int  encDelta = 0;      // エンコーダ増減の累積

// ---- チャタリング除去用前回値 ----
int preSw1 = HIGH;
int preSw2 = HIGH;
int preTsw = HIGH;
int preEncState = 0;   // エンコーダ グレイコード前回値

// ---- アプリケーション状態 ----
Status state = IN_YEAR;
bool   isReiwa = false;  // false=平成, true=令和
int    inputJP = 1;      // 和暦年の入力値
int    inputMonth = 1;
int    inputDay = 1;
int    angelNumber = 0;

// ---- 7SEG表示値（ちらつき防止） ----
int prevLeft = -1;
int prevRight = -1;

// ============================================================
//  ISR : Timer3 CTC (1ms ごとに呼ばれる、6回に1回スキャン)
// ============================================================
ISR(TIMER3_COMPA_vect) {
	if (++irqCount < 6) return;
	irqCount = 0;

	// ----- スイッチ読み取り -----
	int sw1Now = digitalRead(_USER_CON_5PIN);
	int sw2Now = digitalRead(_USER_CON_3PIN);
	int tswNow = digitalRead(_USER_CON_4PIN);
	int encANow = digitalRead(_USER_CON_6PIN);
	int encBNow = digitalRead(_USER_CON_7PIN);

	// SW1 立下りエッジ（チャタリング除去済み）
	if (sw1Now == LOW && preSw1 == HIGH) sw1Pressed = true;

	// SW2 立下りエッジ（チャタリング除去済み）
	if (sw2Now == LOW && preSw2 == HIGH) sw2Pressed = true;

	// トグルSW 変化検出
	if (tswNow != preTsw) {
		tswChanged = true;
		tswRaw = tswNow;
	}

	// エンコーダ: グレイコード 2bit ステートマシン
	// (A,B) の組み合わせ: 00=0, 01=1, 11=3→2へ変換, 10=2
	int encA = (encANow == LOW) ? 1 : 0;
	int encB = (encBNow == LOW) ? 1 : 0;
	int encState = (encA << 1) | encB;

	// CW: 00→10→11→01→00 (+1), CCW: 00→01→11→10→00 (-1)
	static const int8_t encTable[4][4] = {
		//到達状態:  00   01   10   11
		/* 前:00 */ {  0,  -1,  +1,   0 },
		/* 前:01 */ { +1,   0,   0,  -1 },
		/* 前:10 */ { -1,   0,   0,  +1 },
		/* 前:11 */ {  0,  +1,  -1,   0 },
	};
	int d = encTable[preEncState][encState];
	if (d != 0) encDelta += d;

	preSw1 = sw1Now;
	preSw2 = sw2Now;
	preTsw = tswNow;
	preEncState = encState;
}

// ============================================================
//  7SEG表示（ちらつき防止付き）
// ============================================================
void dispNumber(int value) {
	int left = (value / 10) % 10;
	int right = value % 10;
	if (left == prevLeft && right == prevRight) return;  // 変化なければスキップ
	prevLeft = left;
	prevRight = right;
	disp((char)num[left], (char)num[right]);
}

// ============================================================
//  月ごとの日数上限
// ============================================================
int maxDay(int month) {
	switch (month) {
	case 1: case 3: case 5: case 7:
	case 8: case 10: case 12: return 31;
	case 4: case 6: case 9: case 11: return 30;
	case 2: default:                 return 28;
	}
}

// ============================================================
//  エンジェルナンバー算出
//  年・月・日の各桁を順に足し、途中でゾロ目(11,22,...,99)なら確定
// ============================================================
int calcAngel(int year, int month, int day) {
	int digits[8] = {
		(year / 1000) % 10,
		(year / 100) % 10,
		(year / 10) % 10,
		year % 10,
		(month / 10) % 10,
		month % 10,
		(day / 10) % 10,
		day % 10,
	};

	int total = 0;
	for (int i = 0; i < 8; i++) {
		total += digits[i];
		// ゾロ目チェック（11,22,...,99）
		if (total >= 11 && total <= 99 && (total % 11 == 0)) return total;
	}

	// 1桁になるまで桁の和を繰り返す
	while (total >= 10) {
		int tmp = 0;
		int t = total;
		while (t > 0) { tmp += t % 10; t /= 10; }
		total = tmp;
		if (total >= 11 && total <= 99 && (total % 11 == 0)) return total;
	}
	return total;
}

// ============================================================
//  和暦→西暦変換
// ============================================================
int toSeireki(bool reiwa, int jpYear) {
	return reiwa ? (REIWA_OFFSET + jpYear) : (HEISEI_OFFSET + jpYear);
}

// ============================================================
//  元年1月1日のエンジェルナンバー（SW2リセット時・結果表示中）
// ============================================================
int defaultAngel(bool reiwa) {
	return calcAngel(toSeireki(reiwa, 1), 1, 1);
}

// ============================================================
//  状態遷移時の共通処理（強制再描画含む）
// ============================================================
void enterState(Status newState) {
	state = newState;
	lm.b8 = 0;
	lm.color.GBR = LED_COLOR[(int)state];
	prevLeft = -1;  // 強制再描画
	prevRight = -1;
}

// ============================================================
//  年の入力範囲ラップ
// ============================================================
void wrapYear() {
	int lo = HEISEI_MIN;   // 共通下限
	int hi = isReiwa ? REIWA_MAX : HEISEI_MAX;
	if (inputJP < lo) inputJP = hi;
	if (inputJP > hi) inputJP = lo;
}

// ============================================================
//  setup
// ============================================================
void setup() {
	config_init();
	serial_init();

	// DCモータ・ブザー確実停止
	analogWrite(FIN_PIN, 0);
	analogWrite(RIN_PIN, 0);
	digitalWrite(BZ_PIN, LOW);

	// トグルSW初期状態読み込み
	tswRaw = digitalRead(_USER_CON_4PIN);
	preTsw = tswRaw;
	isReiwa = (tswRaw == HIGH);

	// エンコーダ初期グレイコード
	int encA = (digitalRead(_USER_CON_6PIN) == LOW) ? 1 : 0;
	int encB = (digitalRead(_USER_CON_7PIN) == LOW) ? 1 : 0;
	preEncState = (encA << 1) | encB;

	// SW初期値
	preSw1 = digitalRead(_USER_CON_5PIN);
	preSw2 = digitalRead(_USER_CON_3PIN);

	// 初期値設定
	inputJP = 1;
	inputMonth = 1;
	inputDay = 1;

	// 初期状態: IN_YEAR, LED赤
	lm.b8 = 0;
	lm.color.GBR = LED_COLOR[IN_YEAR];
	led_stepmotor(lm.b8);

	// 初期表示: 7SEG "01"
	disp((char)num[0], (char)num[1]);
	prevLeft = 0;
	prevRight = 1;
}

// ============================================================
//  loop
// ============================================================
void loop() {

	// ---- ISRフラグをアトミックにコピーしてクリア ----
	bool doSw1, doSw2, doTsw;
	int  delta;

	noInterrupts();
	doSw1 = sw1Pressed;  sw1Pressed = false;
	doSw2 = sw2Pressed;  sw2Pressed = false;
	doTsw = tswChanged;  tswChanged = false;
	delta = encDelta;    encDelta = 0;
	interrupts();

	// ---- トグルSW処理（年入力状態のみ有効） ----
	if (doTsw && state == IN_YEAR) {
		isReiwa = (tswRaw == HIGH);
		inputJP = 1;        // 下限値に戻す
		prevLeft = -1;       // 強制再描画
		prevRight = -1;
	}

	// ---- SW2 リセット処理 ----
	if (doSw2) {
		if (state == OUT_RESULT) {
			// 結果表示中: トグルSWの状態に応じた元年1月1日のエンジェルナンバーを表示
			angelNumber = defaultAngel(tswRaw == HIGH);
		} else {
			// 入力中: 現在の入力項目を初期値(1)へ
			if (state == IN_YEAR)  inputJP = 1;
			if (state == IN_MONTH) inputMonth = 1;
			if (state == IN_DAY)   inputDay = 1;
		}
		prevLeft = -1;
		prevRight = -1;
	}

	// ---- SW1 確定処理 ----
	if (doSw1) {
		switch (state) {
		case IN_YEAR:
			enterState(IN_MONTH);
			inputMonth = 1;
			break;

		case IN_MONTH:
			enterState(IN_DAY);
			inputDay = 1;
			break;

		case IN_DAY: {
			int year = toSeireki(isReiwa, inputJP);
			angelNumber = calcAngel(year, inputMonth, inputDay);
			enterState(OUT_RESULT);
			break;
		}

		case OUT_RESULT:
			// 先頭に戻る
			inputJP = 1;
			inputMonth = 1;
			inputDay = 1;
			isReiwa = (tswRaw == HIGH);
			enterState(IN_YEAR);
			break;
		}
	}

	// ---- 各ステートの処理 ----
	switch (state) {

		// ===== 年入力 =====
	case IN_YEAR:
		if (delta != 0) {
			inputJP += delta;
			wrapYear();
		}
		dispNumber(inputJP);
		break;

		// ===== 月入力 =====
	case IN_MONTH:
		if (delta != 0) {
			inputMonth += delta;
			if (inputMonth < 1)  inputMonth = 12;
			if (inputMonth > 12) inputMonth = 1;
		}
		dispNumber(inputMonth);
		break;

		// ===== 日入力 =====
	case IN_DAY: {
		int hi = maxDay(inputMonth);
		if (delta != 0) {
			inputDay += delta;
			if (inputDay < 1)  inputDay = hi;
			if (inputDay > hi) inputDay = 1;
		}
		dispNumber(inputDay);
		break;
	}

			   // ===== 結果表示 =====
	case OUT_RESULT:
		dispNumber(angelNumber);
		break;
	}

	// ---- フルカラーLED & モータ出力 ----
	// bit0-3(ステッパ励磁)はクリアして停止維持、bit4-6(RGB)のみ有効
	lm.b8 &= 0x70;
	led_stepmotor(lm.b8);

	// DCモータ・ブザー確実停止
	analogWrite(FIN_PIN, 0);
	analogWrite(RIN_PIN, 0);
	digitalWrite(BZ_PIN, LOW);
}
