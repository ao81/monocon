# `monocon_chuugoku.h` 使用マニュアル

高校生ものづくりコンテスト2026 全国大会向け
対象：Arduino Mega 2560 / ATmega2560 / 16 MHz

---

## 目次

1. [最初に読む内容](#1-最初に読む内容)
2. [ピンと固定ハードウェア](#2-ピンと固定ハードウェア)
3. [デジタル・アナログ入出力](#3-デジタルアナログ入出力)
4. [入力クラス](#4-入力クラス)
5. [出力クラス](#5-出力クラス)
6. [状態・時間管理クラス](#6-状態時間管理クラス)
7. [自由割り込み関数](#7-自由割り込み関数)
8. [組み合わせサンプル](#8-組み合わせサンプル)
9. [内部の仕組み](#9-内部の仕組み)
10. [注意事項とトラブル対策](#10-注意事項とトラブル対策)
11. [クイックリファレンス](#11-クイックリファレンス)

---

# 1. 最初に読む内容

## 1.1 最小プログラム

```cpp
#include "monocon_chuugoku.h"

void setup() {
    begin();
}

void loop() {
    // 競技処理だけを書く
}
```

必ず`setup()`内で一度だけ`begin()`を呼びます。

```cpp
begin();
```

入力、フォトリフレクタ、測距、LED、7セグ表示の内部更新は自動実行されます。

---

## 1.2 オブジェクトはグローバルに作る

入力機器は通常、`setup()`や`loop()`の外で宣言します。

```cpp
di startButton(d1);
pr reflector(a1, 700);
enc dial(18, 19);
```

推奨：

```cpp
di button(d1);

void setup() {
    begin();
}
```

避ける：

```cpp
void loop() {
    di button(d1);  // loopのたびに作らない
}
```

---

## 1.3 自動更新のタイミング

内部更新は主に次のタイミングで行われます。

- ユーザーの`loop()`が1周終了したとき
- `delay()`の待機中

したがって、通常の短い`loop()`では手動更新が不要です。

ただし、次のような永久ループでは更新が止まります。

```cpp
void loop() {
    while (true) {
        // delay()もなく、ここから戻らない
    }
}
```

この間もADC、エンコーダ、ブザー波形など一部の割り込み処理は動きますが、`di`、`pr`、`sok`、LED、7セグの通常更新は止まります。

---

# 2. ピンと固定ハードウェア

## 2.1 短縮ピン名

| 名前 | Arduino Megaピン |
|---|---:|
| `a1` | A0 |
| `a2` | A1 |
| `a3` | A2 |
| `a4` | A3 |
| `d1` | 10 |
| `d2` | 11 |
| `d3` | 12 |
| `d4` | 13 |

例：

```cpp
di button(d1);
vr knob(a1);
```

`H`と`L`も使用できます。

```cpp
if (dr(d1) == H) {
}
```

| 短縮名 | Arduino定数 |
|---|---|
| `H` | `HIGH` |
| `L` | `LOW` |

---

## 2.2 固定出力ピン

| 機能 | 使用ピン |
|---|---|
| 圧電ブザー | D5 |
| 7セグ CLOCK | D6 |
| 7セグ DATA | D7 |
| 7セグ LATCH | D8 |
| フルカラーLED 緑 | D2 |
| フルカラーLED 青 | D3 |
| フルカラーLED 赤 | D4 |
| ステッピングモータ | D28、D26、D24、D22 |
| DCモータ | D44、D46 |
| `PH`入力回路 | D36 |

7セグはハードウェア配線を変更せず、D6・D7・D8から直接ポート操作で出力します。

---

# 3. デジタル・アナログ入出力

## 3.1 `dr()`：高速デジタル入力

```cpp
int value = dr(pin);
```

例：

```cpp
if (dr(d1) == H) {
    led(G);
}
```

`digitalRead()`の短縮・高速版です。

---

## 3.2 `dw()`：高速デジタル出力

```cpp
dw(pin, H);
dw(pin, L);
```

任意の出力ピンを使う場合は、先に`pinMode()`を設定します。

```cpp
void setup() {
    begin();
    pinMode(9, OUTPUT);
}

void loop() {
    dw(9, H);
}
```

---

## 3.3 `ar()`：非同期アナログ入力

```cpp
int value = ar(a1);
```

返り値は通常0～1023です。

```cpp
void loop() {
    dp.n(ar(a1));
}
```

`ar()`はその場でADC変換完了を待ちません。最初に呼ばれたピンはADCの巡回対象へ登録され、以後は最新値を返します。

起動直後や初回登録直後は、まだ測定が完了しておらず0を返す場合があります。

---

## 3.4 `clamp()`：値の範囲制限

```cpp
value = clamp(value, 最小値, 最大値);
```

例：

```cpp
int speed = clamp(ar(a1), 0, 255);
```

```cpp
target += dial.delta();
target = clamp(target, 1, 99);
```

---

# 4. 入力クラス

# 4.1 `di`：デジタル入力・エッジ検出

## 宣言

```cpp
di button(d1);
```

デバウンス時間を変更する場合：

```cpp
di button(d1, 20);
```

第2引数は、入力が同じ状態で安定する必要がある時間です。単位はms、既定値は10msです。

---

## 現在の状態

```cpp
if (button) {
    // HIGHのとき
}
```

または、

```cpp
if (button.level() == H) {
}
```

---

## LOWからHIGHへの変化

```cpp
if (button.ltoh()) {
    // 立ち上がり時に1回
}
```

## HIGHからLOWへの変化

```cpp
if (button.htol()) {
    // 立ち下がり時に1回
}
```

エッジフラグは次の規則で消えます。

- `ltoh()`または`htol()`で読み取ると、その場で消える
- 読まなくても、確認可能な`loop()`を1回経過すると自動的に消える
- `delay()`中に発生したエッジは、次の`loop()`まで保持される

同じ`loop()`内で2回読んだ場合、1回目だけ`true`です。

```cpp
if (button.ltoh()) {
    // 実行される
}

if (button.ltoh()) {
    // 同じエッジはすでに消費済み
}
```

---

## 長押し判定

```cpp
if (button.held(1000, H)) {
    // HIGHが1秒続いたとき1回だけ
}
```

LOW状態の長押し：

```cpp
if (button.held(1000, L)) {
}
```

入力状態が一度変わるまで、同じ長押しでは再び`true`になりません。

---

## 使用例

```cpp
#include "monocon_chuugoku.h"

di button(d1);

void setup() {
    begin();
}

void loop() {
    if (button.ltoh()) {
        led(G);
        bz(1500, 100);
    }

    if (button.htol()) {
        led(R);
    }
}
```

回路が「通常HIGH、押すとLOW」の場合、押した瞬間は`htol()`です。実際の回路極性に合わせて選択してください。

---

# 4.2 `pr`：フォトリフレクタ

## 宣言

```cpp
pr reflector(a1);
```

しきい値を指定：

```cpp
pr reflector(a1, 700);
```

しきい値と安定時間を指定：

```cpp
pr reflector(a1, 700, 10);
```

```cpp
pr(アナログピン, しきい値, 安定時間ms)
```

ADC値がしきい値より大きいとHIGHになります。

```cpp
if (reflector) {
    // ADC値 > 700
}
```

`di`と同じ機能が使えます。

```cpp
reflector.level();
reflector.ltoh();
reflector.htol();
reflector.held(500, H);
```

---

## 使用例

```cpp
#include "monocon_chuugoku.h"

pr reflector(a1, 700);

void setup() {
    begin();
}

void loop() {
    if (reflector.ltoh()) {
        bz(1200, 50);
    }

    led(reflector ? G : R);
}
```

---

# 4.3 `sok`：測距モジュール

## 宣言

```cpp
sok distance(a1);
```

## 読み取れる値

| 値 | 内容 |
|---|---|
| `distance._raw` | ADC割り込みが保存した最新値 |
| `distance.raw` | 5点中央値処理後のADC値 |
| `distance.mm` | ミリメートル換算値 |
| `distance.cm` | センチメートル換算値 |

使用例：

```cpp
dp.n(distance.mm);
```

```cpp
dp.f(distance.cm);
```

距離変換には次の固定校正値が使われています。

| ADC値 | 距離 |
|---:|---:|
| 450 | 40mm |
| 380 | 150mm |
| 260 | 300mm |
| 60 | 500mm |

使用するセンサや取付条件が異なる場合は、実測して校正値を変更してください。

---

## 単体表示サンプル

```cpp
#include "monocon_chuugoku.h"

sok distance(a1);

void setup() {
    begin();
}

void loop() {
    dp.n(distance.mm);

    if (distance.mm < 100) {
        led(R);
    } else {
        led(G);
    }
}
```

起動直後は5サンプルがそろうまで値が0の場合があります。

---

# 4.4 `vr`：半固定抵抗

## 宣言

```cpp
vr knob(a1);
```

生の値：

```cpp
int value = monoconAtomicReadInt(&knob.raw);
```

通常は`to()`を使う方が簡単です。

```cpp
int speed = knob.to(0, 255);
```

0～1023のADC値を指定範囲へ変換します。

```cpp
int angle = knob.to(-90, 90);
```

---

## 使用例

```cpp
#include "monocon_chuugoku.h"

vr knob(a1);

void setup() {
    begin();
}

void loop() {
    int speed = knob.to(0, 255);

    dm.cw(speed);
    dp.n(speed);
}
```

---

# 4.5 `joy`：ジョイスティック

## 宣言

```cpp
joy stick(a1, a2);
```

生の値：

```cpp
int x = monoconAtomicReadInt(&stick.x);
int y = monoconAtomicReadInt(&stick.y);
```

---

## 4方向判定

```cpp
int direction = stick.dir(4);
```

| 戻り値 | 方向 |
|---:|---|
| `0` | 上 |
| `1` | 右 |
| `2` | 下 |
| `3` | 左 |
| `-1` | 中央 |

---

## 8方向判定

```cpp
int direction = stick.dir(8);
```

| 戻り値 | 方向 |
|---:|---|
| `0` | 上 |
| `1` | 右上 |
| `2` | 右 |
| `3` | 右下 |
| `4` | 下 |
| `5` | 左下 |
| `6` | 左 |
| `7` | 左上 |
| `-1` | 中央 |

回転：

```cpp
stick.dir(4, 1);  // 判定方向を90度回転
```

反転：

```cpp
stick.dir(4, 0, true);
```

---

## 使用例

```cpp
#include "monocon_chuugoku.h"

joy stick(a1, a2);

void setup() {
    begin();
}

void loop() {
    switch (stick.dir(4)) {
        case 0:
            dm.cw(200);
            dp.s("UP_");
            break;

        case 2:
            dm.ccw(200);
            dp.s("dn_");
            break;

        default:
            dm.br();
            dp.off();
            break;
    }
}
```

4方向・8方向は整数演算で高速に判定されます。それ以外の分割数では`atan2()`が使用されます。

---

# 4.6 `enc`：ロータリーエンコーダ

## 宣言

```cpp
enc dial(18, 19);
```

回転方向を逆にする場合：

```cpp
enc dial(18, 19, false);
```

エンコーダは10kHzのTimer1割り込みで読み取られます。

---

## 累積値

```cpp
int32_t value = dial.count();
```

## 前回からの変化量

```cpp
int difference = dial.delta();
```

## 値を設定

```cpp
dial.set();       // 0
dial.set(50);     // 50
```

## 指定範囲で制限

```cpp
int value = dial.clampTo(1, 99);
```

## 指定範囲で循環

```cpp
int value = dial.loopTo(0, 9);
```

0より前は9、9より後は0になります。

---

## 使用例

```cpp
#include "monocon_chuugoku.h"

enc dial(18, 19);

void setup() {
    begin();
    dial.set(1);
}

void loop() {
    int target = dial.clampTo(1, 99);
    dp.n(target);
}
```

---

# 5. 出力クラス

# 5.1 `led`：フルカラーLED

使用可能な色：

| 定数 | 色 |
|---|---|
| `R` | 赤 |
| `G` | 緑 |
| `B` | 青 |
| `GR` | 赤＋緑 |
| `GB` | 緑＋青 |
| `BR` | 青＋赤 |
| `GBR` | 全色 |
| `0` | 消灯 |

点灯：

```cpp
led(G);
```

明るさ指定：

```cpp
led(G, 50);
```

第2引数は0～100%です。

消灯：

```cpp
led(0);
```

---

# 5.2 `dp`：3桁7セグメントLED

## 消灯

```cpp
dp.off();
```

## 文字表示

```cpp
dp.s("End");
dp.s("run");
dp.s("1_2");
```

最大3文字です。`_`は空白として使用できます。

7セグの構造上、アルファベットは近似表示です。

---

## 整数表示

```cpp
dp.n(123);
```

ゼロ埋め：

```cpp
dp.n(7, true);  // 007
```

左寄せ：

```cpp
dp.n(7, false, true);
```

表示可能範囲の目安：

- 正数：0～999
- 負数：-99～-1

範囲外は表示可能範囲へ制限されます。

---

## 小数表示

```cpp
dp.f(1.23);
dp.f(12.3);
dp.f(123.0);
```

3桁に収まるよう小数点位置が自動調整されます。

---

## 明るさ

全桁同じ：

```cpp
dp.o(50);
```

桁別：

```cpp
dp.o(100, 50, 20);
```

---

## 生パターン表示

```cpp
dp(seg[1], seg[2], seg[3]);
```

小数点：

```cpp
dp(seg[1] | SEG_DOT, seg[2], seg[3]);
```

定数：

```cpp
SEG_DOT
SEG_MINUS
SEG_NONE
```

---

# 5.3 `dm`：DCモータ

正転：

```cpp
dm.cw(200);
```

逆転：

```cpp
dm.ccw(200);
```

速度は0～255です。範囲外は自動制限されます。

ブレーキ停止：

```cpp
dm.br();
```

フリー停止：

```cpp
dm.fr();
```

現在の方向：

```cpp
dm.now
```

| 値 | 状態 |
|---:|---|
| `1` | 正転 |
| `-1` | 逆転 |
| `0` | 停止 |

---

# 5.4 `sm`：ステッピングモータ

1ステップ正転：

```cpp
sm.cw();
```

1ステップ逆転：

```cpp
sm.ccw();
```

現在の位相で保持：

```cpp
sm.br();
```

励磁解除：

```cpp
sm.fr();
```

呼び出す間隔が回転速度になります。通常は`iv`と組み合わせます。

```cpp
iv stepTimer;

void loop() {
    if (stepTimer(5)) {
        sm.cw();
    }
}
```

---

# 5.5 `bz`：圧電ブザー

連続音：

```cpp
bz(1000);
```

指定時間だけ鳴らす：

```cpp
bz(1500, 200);
```

停止：

```cpp
bz.off();
```

重要：時間指定音は、呼ぶたびに残り時間が再スタートします。

避ける：

```cpp
void loop() {
    bz(1000, 200);  // 毎loop再スタートし、停止しにくい
}
```

推奨：

```cpp
if (button.ltoh()) {
    bz(1000, 200);
}
```

または、状態入場時に一度だけ呼びます。

```cpp
if (q.in()) {
    bz(1000, 200);
}
```

---

# 6. 状態・時間管理クラス

# 6.1 `Seq`：状態遷移

`Seq`は、`enum`や`switch`を短く書くための状態管理クラスです。

## 基本形

```cpp
Seq q;

void loop() {
    if (q) {
        // 状態0
    }

    if (q) {
        // 状態1
    }

    if (q) {
        // 状態2
    }
}
```

`q.top()`は不要です。最初の`if (q)`で、新しい`loop()`を自動判定します。

---

## 次の状態へ進む

```cpp
q.next();
```

最後の状態から`next()`すると、状態0へ戻ります。

```cpp
if (q) {
    if (button.ltoh()) {
        q.next();
    }
}
```

状態を移動した同じ`loop()`内では、後続の状態は実行されません。次の`loop()`から新しい状態が動きます。

---

## 前の状態へ戻る

```cpp
q.prev();
```

状態0から`prev()`すると、最後の状態へ移動します。

---

## 指定状態へ移動

```cpp
q.to(0);
q.to(2);
```

同じ状態を指定した場合は何もしません。

---

## 現在の状態を再スタート

```cpp
q.restart();
```

現在状態の経過時間を0にし、`in()`を再び有効にします。

---

## 状態へ入った瞬間

```cpp
if (q.in()) {
    // 状態へ入って最初の1回だけ
}
```

初期状態0でも、最初に`in()`を呼ぶと1回だけ`true`です。

---

## 状態を出た瞬間

```cpp
q.next();

if (q.out()) {
    // 遷移を行った同じloop内で1回だけ
}
```

`out()`は遷移した同じ`loop()`内で使用します。次の`loop()`へ移ると退出イベントは破棄されます。

---

## 状態番号

```cpp
if (q.is(2)) {
}
```

```cpp
int state = q.now();
```

検出済み状態数：

```cpp
int count = q.steps();
```

`steps()`は最初の`loop()`が終了するまで0の場合があります。

---

## 状態の経過時間

```cpp
uint32_t time = q.elapsed();
```

指定時間経過：

```cpp
if (q.after(1000)) {
    q.next();
}
```

---

## Seqサンプル

```cpp
#include "monocon_chuugoku.h"

di button(d1);
Seq q;

void setup() {
    begin();
}

void loop() {
    if (q) {
        // 待機
        led(R);
        dm.fr();
        dp.s("rdy");

        if (button.ltoh()) {
            q.next();
        }
    }

    if (q) {
        // 2秒運転
        if (q.in()) {
            led(G);
            dm.cw(200);
            dp.s("run");
        }

        if (q.after(2000)) {
            q.next();
        }
    }

    if (q) {
        // 完了
        if (q.in()) {
            dm.br();
            led(B);
            dp.s("End");
            bz(1500, 200);
        }

        if (button.ltoh()) {
            q.to(0);
        }
    }
}
```

---

# 6.2 `iv`：一定間隔タイマー

一定時間ごとに処理を実行します。

```cpp
iv timer;

void loop() {
    if (timer(100)) {
        // 約100msごと
    }
}
```

---

## 周期を数え直す

```cpp
timer.reset();
```

`reset()`した時点から次の周期を数えます。

---

## 一時停止

```cpp
timer.wait();
```

再開：

```cpp
timer.go();
```

停止中の時間は周期へ含まれません。停止した時点の残り時間を維持して再開します。

停止中か確認：

```cpp
if (timer.isWait()) {
}
```

---

## 注意

`iv`は遅れた分をまとめて複数回返しません。実行が遅れた場合、その時点を新しい基準にして1回だけ`true`になります。

```cpp
if (timer(0)) {
    // すべての呼び出しでtrue
}
```

通常は1以上を指定してください。

---

## 点滅サンプル

```cpp
#include "monocon_chuugoku.h"

iv blink;
bool on = false;

void setup() {
    begin();
}

void loop() {
    if (blink(500)) {
        on = !on;
        led(on ? G : 0);
    }
}
```

---

# 6.3 `ti`：ワンショットタイマー

指定時間後に一度だけ`true`を返します。

```cpp
ti timer;

timer.start(1000);
```

完了確認：

```cpp
if (timer.done()) {
    // 1秒後に1回だけ
}
```

`done()`が`true`を返すと、自動的に停止します。

---

## 操作

```cpp
timer.stop();
```

```cpp
if (timer.active()) {
}
```

```cpp
uint32_t left = timer.remain();
```

安全に指定できる最大時間は約24.8日です。これを超える値は約24.8日へ制限されます。

---

## 警告表示サンプル

```cpp
#include "monocon_chuugoku.h"

di button(d1);
ti warning;

void setup() {
    begin();
}

void loop() {
    if (button.ltoh()) {
        led(R);
        warning.start(500);
    }

    if (warning.done()) {
        led(G);
    }
}
```

---

# 6.4 `Sw`：ストップウォッチ

開始：

```cpp
Sw watch;
watch.start();
```

経過時間：

```cpp
uint32_t time = watch.ms();
```

短縮：

```cpp
uint32_t time = watch();
```

停止：

```cpp
watch.stop();
```

停止後は、その時点の値で固定されます。

リセット：

```cpp
watch.reset();
```

動作中か確認：

```cpp
if (watch.running()) {
}
```

`start()`は以前の値を消して0から開始します。停止後の続きを再開する機能ではありません。

---

## 計測サンプル

```cpp
#include "monocon_chuugoku.h"

di button(d1);
Sw watch;

void setup() {
    begin();
}

void loop() {
    if (button.ltoh()) {
        if (watch.running()) {
            watch.stop();
        } else {
            watch.start();
        }
    }

    dp.n(watch() / 100);
}
```

---

# 7. 自由割り込み関数

Timer1の10kHz割り込み内で、自由な短い処理を実行できます。

`useir`をヘッダより前で定義します。

```cpp
#define useir
#include "monocon_chuugoku.h"

volatile bool flag = false;

void ir() {
    flag = true;
}

void setup() {
    begin();
}

void loop() {
    if (flag) {
        noInterrupts();
        flag = false;
        interrupts();

        // 通常処理
    }
}
```

`ir()`は100µsごとに、割り込み禁止状態で実行されます。

## `ir()`で適する処理

- フラグ更新
- 小さな整数の加算
- 数命令の直接ポート操作
- 非常に短い条件分岐

## 避ける処理

- `delay()`
- `Serial.print()`
- `millis()`待ちループ
- 浮動小数点演算
- 長い`for`や`while`
- 7セグへの文字列処理
- `bz(f, ms)`などの重い設定処理

---

# 8. 組み合わせサンプル

# 8.1 部品カウント装置

使用機能：

- タクトスイッチ
- トグルスイッチ
- フォトインタラプタ
- フォトリフレクタ
- ロータリーエンコーダ
- DCモータ
- LED
- 7セグ
- ブザー
- `Seq`
- `ti`

```cpp
#include "monocon_chuugoku.h"

di startButton(d1);
di resetButton(d2);
di modeSwitch(d3);
di interrupter(d4);

pr reflector(a1, 700);
enc dial(18, 19);

Seq q;
ti errorTimer;

int target = 1;
int count = 0;

void setup() {
    begin();
    dial.set(1);
}

void loop() {
    // どの状態でもリセット可能
    if (resetButton.htol()) {
        count = 0;
        dm.fr();
        bz.off();

        if (modeSwitch) {
            q.to(1);
        } else {
            q.to(0);
        }

        return;
    }

    // トグルOFFなら設定状態へ戻す
    if (!modeSwitch && !q.is(0)) {
        q.to(0);
        return;
    }

    if (q) {
        // 状態0：目標個数設定
        target = dial.clampTo(1, 99);

        dp.n(target);
        led(B);
        dm.fr();

        if (modeSwitch) {
            count = 0;
            q.next();
        }
    }

    if (q) {
        // 状態1：運転待機
        dp.n(count);
        led(R);
        dm.fr();

        if (startButton.htol()) {
            q.next();
        }
    }

    if (q) {
        // 状態2：運転
        dm.cw(200);
        dp.n(count);

        if (interrupter.ltoh()) {
            if (reflector) {
                ++count;
            } else {
                bz(800, 100);
                errorTimer.start(100);
            }
        }

        led(errorTimer.active() ? R : G);

        // remain()が0になってもactive()はdone()までtrueなので、
        // 完了を必ず確認する
        errorTimer.done();

        if (count >= target) {
            q.next();
        }
    }

    if (q) {
        // 状態3：完了
        if (q.in()) {
            dm.br();
            led(R);
            dp.s("End");
            bz(1500, 300);
        }
    }
}
```

---

# 8.2 ステッピングモータ自動往復

使用機能：

- タクトスイッチ
- `Seq`
- `iv`
- ステッピングモータ
- LED
- 7セグ

```cpp
#include "monocon_chuugoku.h"

di startButton(d1);
Seq q;
iv stepTimer;

int steps = 0;

void setup() {
    begin();
}

void loop() {
    if (q) {
        // 待機
        led(R);
        dp.s("rdy");
        sm.fr();

        if (startButton.ltoh()) {
            steps = 0;
            stepTimer.reset();
            q.next();
        }
    }

    if (q) {
        // 正転200ステップ
        led(G);
        dp.n(steps);

        if (stepTimer(5)) {
            sm.cw();

            if (++steps >= 200) {
                steps = 0;
                q.next();
            }
        }
    }

    if (q) {
        // 逆転200ステップ
        led(B);
        dp.n(steps);

        if (stepTimer(5)) {
            sm.ccw();

            if (++steps >= 200) {
                q.next();
            }
        }
    }

    if (q) {
        // 完了
        if (q.in()) {
            sm.br();
            dp.s("End");
            bz(1200, 200);
        }

        if (startButton.ltoh()) {
            q.to(0);
        }
    }
}
```

---

# 8.3 反応時間測定

使用機能：

- タクトスイッチ2個
- `Seq`
- `ti`
- `Sw`
- LED
- 7セグ
- ブザー

```cpp
#include "monocon_chuugoku.h"

di startButton(d1);
di reactionButton(d2);

Seq q;
ti waitTimer;
Sw reactionTime;

void setup() {
    begin();
}

void loop() {
    if (q) {
        // 開始待ち
        led(R);
        dp.s("rdy");

        if (startButton.ltoh()) {
            waitTimer.start(2000);
            q.next();
        }
    }

    if (q) {
        // 合図待ち
        led(0);
        dp.off();

        if (reactionButton.ltoh()) {
            // フライング
            bz(300, 300);
            q.to(0);
        } else if (waitTimer.done()) {
            led(G);
            bz(1500, 50);
            reactionTime.start();
            q.next();
        }
    }

    if (q) {
        // 反応待ち
        dp.n(reactionTime() / 10);

        if (reactionButton.ltoh()) {
            reactionTime.stop();
            q.next();
        }
    }

    if (q) {
        // 結果表示
        if (q.in()) {
            led(B);
            dp.n(reactionTime() / 10);
        }

        if (startButton.ltoh()) {
            reactionTime.reset();
            q.to(0);
        }
    }
}
```

7セグは3桁のため、この例では10ms単位で表示しています。

---

# 9. 内部の仕組み

この章の内容は、通常の競技プログラムで直接操作する必要はありません。

## 9.1 使用タイマー

| タイマー | 用途 |
|---|---|
| Timer0 | Arduino標準の`millis()`、`micros()`、`delay()` |
| Timer1 | 10kHzエンコーダ読み取り、任意の`ir()` |
| Timer2 | 1kHz内部時刻、更新要求、時限ブザー管理 |
| Timer3 | ブザーのハードウェア波形生成 |
| Timer5 | DCモータPWM |

これらのタイマーを書き換える他ライブラリとは競合する場合があります。

---

## 9.2 自動更新

Timer2 ISRは1msごとに、重い処理を直接実行せず更新要求だけを立てます。

通常処理側で次を更新します。

- `di`
- `pr`
- `sok`
- `led`
- `dp`

更新処理は`serialEventRun()`から、各`loop()`終了時に呼ばれます。

`yield()`からも呼ばれるため、`delay()`中も更新されます。

---

## 9.3 エッジの寿命

`di`と`pr`のエッジは、検出直後に消えるのではありません。

1. 内部更新でエッジを検出
2. 次のユーザー`loop()`で読み取り可能
3. `ltoh()`／`htol()`で読めば即消費
4. 読まなくても、その`loop()`終了後に自動失効

この方式により、1msだけのフラグより見逃しにくく、押しっぱなしによる連続実行も防ぎます。

---

## 9.4 ADC

ADCは完了割り込みで連続スキャンします。

- 変換完了待ちループなし
- 最大16チャンネル
- ADCクロック125kHz
- 10ビット精度優先
- 登録された入力だけを巡回

ADC使用クラス：

- `pr`
- `sok`
- `vr`
- `joy`
- `ar()`

---

## 9.5 登録上限

| クラス・機能 | 最大数 |
|---|---:|
| ADC登録 | 16 |
| `di` | 8 |
| `pr` | 8 |
| `sok` | 2 |
| `enc` | 4 |

上限を超えたオブジェクトは、内部更新リストへ追加されません。

---

## 9.6 7セグ転送

D6・D7・D8を使い、24ビットをソフトウェアで転送します。

- 配線変更不要
- ループを使わず8ビット処理を展開
- 表示内容が変わったときだけ転送
- ISR外で実行

---

## 9.7 ブザー

Timer3のOC3Aハードウェア出力を使います。音の各エッジはCPUではなくタイマーが生成します。

周波数変更時はTimer3を停止し、カウンタを0へ戻してから既知の位相で再開します。

---

# 10. 注意事項とトラブル対策

## 10.1 ヘッダは1か所からだけインクルードする

このヘッダはグローバル変数とISRの実体を含みます。

通常の1つの`.ino`では問題ありませんが、複数の`.cpp`から同時にインクルードすると多重定義になる可能性があります。

---

## 10.2 `serialEvent()`は自動実行されない

このヘッダは内部更新のために`serialEventRun()`を定義しています。

Arduino標準の次の関数は、自動的には呼ばれません。

```cpp
void serialEvent() {
}
```

Serial自体は通常どおり使えます。

```cpp
void loop() {
    if (Serial.available()) {
        int c = Serial.read();
    }
}
```

---

## 10.3 他の`yield()`定義と競合する可能性

このヘッダは`delay()`中の更新のために`yield()`を定義します。

別ライブラリも`yield()`を強く定義している場合、リンクエラーや機能競合が起きる可能性があります。

---

## 10.4 `delayMicroseconds()`中は自動更新されない

`delay()`は`yield()`を呼びますが、`delayMicroseconds()`は通常呼びません。

長いマイクロ秒待機を繰り返すと、通常更新が遅れる可能性があります。

---

## 10.5 入力に内部プルアップは設定されない

`begin()`は固定入力をINPUTにしますが、内部プルアップを有効にしません。

回路側の抵抗を使用するか、必要なピンに明示的に設定してください。

```cpp
void setup() {
    begin();
    pinMode(d1, INPUT_PULLUP);
}
```

ただし、外部回路との接続方式を確認してから使用してください。

---

## 10.6 `bz(f, ms)`を毎loop呼ばない

毎回タイマーが再スタートします。エッジ、`q.in()`、条件変化時など、一度だけ実行される場所で呼びます。

---

## 10.7 ステッピングモータを毎loop回さない

```cpp
void loop() {
    sm.cw();  // loopが速すぎて脱調する可能性
}
```

`iv`で適切な間隔を作ります。

```cpp
if (stepTimer(5)) {
    sm.cw();
}
```

---

## 10.8 `ti.active()`だけでは期限切れを停止できない

期限が来ても、`done()`を呼ぶまでは`running_`が解除されません。

推奨：

```cpp
if (timer.done()) {
}
```

残り時間が0になったことだけを確認する場合でも、必要に応じて`done()`を呼んで完了状態へ移してください。

---

# 11. クイックリファレンス

```cpp
// 初期化
begin();

// 短縮ピン
a1, a2, a3, a4
d1, d2, d3, d4
H, L

// GPIO
dr(pin);
dw(pin, H);
dw(pin, L);
ar(pin);
clamp(value, lo, hi);

// デジタル入力
di button(d1);
button.level();
button.ltoh();
button.htol();
button.held(1000, H);

// フォトリフレクタ
pr reflector(a1, 700);
reflector.ltoh();
reflector.htol();

// 測距
sok distance(a1);
distance.raw;
distance.mm;
distance.cm;

// 半固定抵抗
vr knob(a1);
knob.to(0, 255);

// ジョイスティック
joy stick(a1, a2);
stick.dir(4);
stick.dir(8);

// エンコーダ
enc dial(18, 19);
dial.count();
dial.delta();
dial.set(0);
dial.clampTo(1, 99);
dial.loopTo(0, 9);

// LED
led(R);
led(G, 50);
led(0);

// 7セグ
dp.off();
dp.s("End");
dp.n(123);
dp.f(1.23);
dp.o(50);

// DCモータ
dm.cw(255);
dm.ccw(255);
dm.br();
dm.fr();

// ステッピングモータ
sm.cw();
sm.ccw();
sm.br();
sm.fr();

// ブザー
bz(1000);
bz(1000, 200);
bz.off();

// 状態管理
Seq q;
if (q) {}
q.next();
q.prev();
q.to(0);
q.restart();
q.in();
q.out();
q.is(1);
q.now();
q.steps();
q.elapsed();
q.after(1000);

// 一定間隔
iv interval;
if (interval(100)) {}
interval.reset();
interval.wait();
interval.go();
interval.isWait();

// ワンショット
ti timer;
timer.start(1000);
timer.stop();
timer.active();
timer.done();
timer.remain();

// ストップウォッチ
Sw watch;
watch.start();
watch.stop();
watch.reset();
watch.running();
watch.ms();
watch();
```

---

# 本番前チェックリスト

- [ ] `setup()`で`begin()`を1回呼んだ
- [ ] 入力オブジェクトをグローバルに作った
- [ ] スイッチのHIGH／LOW極性を実機確認した
- [ ] フォトリフレクタのしきい値を実測した
- [ ] 測距モジュールの校正値を確認した
- [ ] `bz(f, ms)`を毎loop呼んでいない
- [ ] ステッピングモータに適切な間隔を設けた
- [ ] Timer1・2・3・5を使う他ライブラリと競合していない
- [ ] 永久ループで自動更新を止めていない
- [ ] `ir()`を使用する場合、処理を十分短くした
- [ ] 7セグがD6・D7・D8へ接続されている
- [ ] DCモータの正転・逆転方向を確認した
