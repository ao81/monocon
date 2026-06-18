<!-- markdownlint-disable -->

### ジョイスティックの入力を {DIV} 方向に分割する
> DIV=4 なら、上が0,右が1,下が2,左が3 のように上が0としてcw方向に1ずつ増える。<br>
> 中央から半径約141（= sqrt(20000)）以内は入力なしとみなして-1を返す。

```c++

int getdir(int xx, int yy, int DIV) {
	long dx = xx - 511, dy = yy - 511;
	if (dx * dx + dy * dy < 20000) return -1;
	return (int)((atan2(dx, dy) + 2 * PI + PI / DIV) / (2 * PI / DIV)) % DIV;
}

```

<br>

### ステッピングモーターの最短移動方向を求める
> -60 ~ 60 の値を返す。<br>
> 引数は 0 ~ 119。

```c++

int getmove(int from, int to) {
	int diff = to - from;
	if (diff > 60) diff -= 120;
	if (diff < -60) diff += 120;
	return diff;
}

```

<br>

### DCモーターの制御関数

```c++

void cw(int spd) {
	analogWrite(FIN_PIN, spd);
	digitalWrite(RIN_PIN, LOW);
}

void ccw(int spd) {
	digitalWrite(FIN_PIN, LOW);
	analogWrite(RIN_PIN, spd);
}

void stop() {
	digitalWrite(FIN_PIN, HIGH);
	digitalWrite(RIN_PIN, HIGH);
}

void free() {
	digitalWrite(FIN_PIN, LOW);
	digitalWrite(RIN_PIN, LOW);
}

```

<br>

### テンプレート
> x,y...ジョイスティック, tsw...トグルスイッチ, sw...タクトスイッチ, ph...フォトインタラプタ<br>
> x=pin2, y=pin1, tsw=pin5, sw=pin4, ph=pin3

```c++

#USE_TIMER3
#include "mono_con.h"

int tsw, sw, ph;
bool r = true;

ISR(TIMER3_COMPA_vect) {
	static word in = 0;
	if (in++ > 5) {
		in = 0;
		r = true;
	}
}

void setup() {
	config_init();
	serial_init();
}

void loop() {
	if (r) {
		r = false;

		x = analogRead(pin2);
		y = analogRead(pin1);
		tsw = digitalRead(pin5);
		sw = digitalRead(pin4);
		ph = digitalRead(pin3);
	}


}

```

<br>

<!--
###

```c++



```

<br>
-->
