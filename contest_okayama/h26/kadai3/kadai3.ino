// 17:03

// sw1をsw, sw2をjs↓とする
// ledの点灯回数0:なし　1:赤　2:黄　3:青　4:緑　とする

#define USE_TIMER3_ISR
#include "mono_con.h"

const int ledPara[5] = { B000, B001, B101, B010, B100 };
int count = 0;

const int lo = 0 + 200;
const int hi = 1023 - 200;

int ypos, sw;
int preypos, presw;
bool jsps = false, swps = false;

word in;

void onedisp(int n, int m) {
	static int pren = 0x00, prem = 0x00;
	if (pren != n || prem != m) {
		pren = n, prem = m;
		disp(n, m);
	}
}

ISR(TIMER3_COMPA_vect) {
	if (in++ > 5) {
		in = 0;

		ypos = analogRead(A2);
		sw = digitalRead(_USER_CON_4PIN);

		if (ypos > hi && preypos <= hi) jsps = true;
		if (sw == LOW && presw == HIGH) swps = true;

		preypos = ypos;
		presw = sw;
	}
}

void setup() {
	config_init();
	serial_init();

	ypos = preypos = analogRead(A2);
	sw = presw = digitalRead(_USER_CON_4PIN);

	disp(0x00, num[0]);
}

void loop() {
	if (swps) {
		swps = false;
		if (count < 9) count++;
	}

	if (jsps) {
		jsps = false;
		count = 0;
	}

	if (count <= 1) {
		lm.color.GBR = ledPara[0];
	} else if (count <= 3) {
		lm.color.GBR = ledPara[1];
	} else if (count <= 5) {
		lm.color.GBR = ledPara[2];
	} else if (count <= 7) {
		lm.color.GBR = ledPara[3];
	} else {
		lm.color.GBR = ledPara[4];
	}

	led_stepmotor(lm.b8);
	onedisp(0x00, num[count]);
}