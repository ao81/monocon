#include "monocon_chuugoku.h"

void setup() {
	begin();
}

void loop() {
	// フォトリフレクタ
	int pr = ar(a2);
	if (pr < 950) led(R);
	else led(0);

	// 測距モジュール
	static iv t;
	if (t(100)) {
		int sk = in.sok(a1);
		// dispn(sk);
	}

	// ロータリーエンコーダ
	int n = in.enc(a3, a4);
	dispn(n);
}
