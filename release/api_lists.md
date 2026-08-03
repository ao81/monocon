# monocon_chuugoku.h 公開APIチェックリスト

## 1. 独立関数

- [ ] `void ir()` — 条件付き公開コールバック — `useir`定義時にユーザーが実装
- `static void board_detail::digitalInputInit()` — 内部
- [ ] `template <typename T, typename U, typename V> inline T clamp(T v, U lo, V hi)` — 公開
- [ ] `template <typename T, typename U, typename V> inline T wrap(T value, U low, V high)` — 公開
- `inline uint32_t board_detail::atomicMillis()` — 内部
- `inline int board_detail::atomicReadInt(volatile const int* p)` — 内部
- `bool board_detail::adcReg(uint8_t pin, volatile int* dst)` — 内部
- `bool board_detail::adcUnreg(volatile int* dst)` — 内部
- [ ] `int ar(uint8_t pin)` — 公開
- [ ] `inline int dr(uint8_t pin)` — 公開
- [ ] `inline void dw(uint8_t pin, uint8_t val)` — 公開
- `bool board_detail::adcReady(volatile const int* dst)` — 内部
- `void board_detail::service()` — 内部
- `void board_detail::serviceEvents(uint32_t epoch)` — 内部
- `void board_detail::commitOutputs()` — 内部
- `void board_detail::begin()` — 内部
- `inline void board_detail::lockDigitalInputs()` — 内部
- `inline uint8_t board_detail::percentToByte(int p)` — 内部
- `inline void board_detail::shiftBit(uint8_t high)` — 内部
- `inline void board_detail::shiftByte(uint8_t v)` — 内部
- `inline void board_detail::writeDisplay3(uint8_t a, uint8_t b, uint8_t c)` — 内部
- `inline void board_detail::compareSwap(int& a, int& b)` — 内部
- `inline int board_detail::median5(int a, int b, int c, int d, int e)` — 内部
- `void ADC_vect(void)` — 内部ISR
- `void PCINT0_vect(void)` — 内部ISR
- `void PCINT1_vect(void)` — 内部ISR
- `void PCINT2_vect(void)` — 内部ISR
- `void TIMER1_COMPA_vect(void)` — 内部ISR
- `void TIMER2_COMPA_vect(void)` — 内部ISR
- `inline uint8_t board_detail::analogChannel(uint8_t pin)` — 内部
- `inline void board_detail::selectAdcSlot(uint8_t index)` — 内部
- `inline void board_detail::startAdcLocked()` — 内部
- `inline void board_detail::adcIsrBody()` — 内部
- `void yield()` — ライブラリ実行基盤
- `void userSetup()` — weakユーザーフック
- `void userLoop()` — weakユーザーフック
- `void setup()` — 内部ラッパー
- `void loop()` — 内部ラッパー
- `inline void debugLine()` — `dbg`時の内部デバッグ補助
- `template <typename T, typename... Rest> inline void debugLine(const T& value, const Rest&... rest)` — `dbg`時の内部デバッグ補助

## 2. クラス・構造体と全メソッド

### struct `Dch`

- 入力の安定値・候補値・継続時間を保持する内部状態構造体

### class `PollLock`

- ポーリング間隔を制限する内部補助クラス

**publicメソッド**

- [ ] `explicit PollLock(uint16_t lock = 10)`
- [ ] `bool due(uint32_t now)`

### class `InEdge`

- `Di`・`Pr`が継承するエッジ／長押し判定基底クラス

**publicメソッド**

- [ ] `bool ltoh()`
- [ ] `bool htol()`
- [ ] `bool level() const`
- [ ] `operator bool() const`
- [ ] `bool held(uint16_t ms, bool lv, bool release = false)`
- [ ] `bool change()`

**protectedメソッド**

- `void pollWith(uint8_t raw, uint32_t now)`
- `void serviceEdges(uint32_t epoch)`
- `explicit InEdge(uint16_t lock = 10)`

### struct `board_detail::RemoveReference`

- 参照修飾を除去する内部型特性

### struct `board_detail::RemoveReference<T&>`

- `T&`用の内部型特性特殊化

### struct `board_detail::RemoveReference<T&&>`

- `T&&`用の内部型特性特殊化

### struct `board_detail::RemoveCv`

- CV修飾を除去する内部型特性

### struct `board_detail::RemoveCv<const T>`

- `const T`用の内部型特性特殊化

### struct `board_detail::RemoveCv<volatile T>`

- `volatile T`用の内部型特性特殊化

### struct `board_detail::RemoveCv<const volatile T>`

- `const volatile T`用の内部型特性特殊化

### struct `board_detail::SignalValueType`

- `sig(...)`が値型を決定する内部型特性

### class `SigBase`

- `SigValue`のイベント登録・寿命管理を行う内部基底クラス

**publicメソッド**

- [ ] `SigBase(const SigBase&) = delete` — 削除定義（呼び出し不可）
- [ ] `SigBase& operator=(const SigBase&) = delete` — 削除定義（呼び出し不可）

**protectedメソッド**

- `SigBase()`
- `~SigBase()`
- `void publishChange()`
- `void publishRelease()`
- `bool takeChange(bool matches = true)`
- `bool takeRelease(bool matches = true)`
- `void clearEvents()`

**privateメソッド**

- `void attach()`
- `void detach()`
- `static void serviceAll(uint32_t epoch)`

### class `SigValue`（public SigBase）

- [ ] 任意型の値を安定化し、変化・方向・保持時間を判定する公開テンプレート

**publicメソッド**

- [ ] `explicit SigValue(uint16_t lock = 10, T tolerance = T())`
- [ ] `T set(T value)`
- [ ] `T update(T value)`
- [ ] `T operator()(T value)`
- [ ] `void reset(T value = T())`
- [ ] `bool initialized() const`
- [ ] `T level() const`
- [ ] `T previous() const`
- [ ] `operator T() const`
- [ ] `bool change()`
- [ ] `bool changed()`
- [ ] `bool change(T from, T to)`
- [ ] `bool from(T value)`
- [ ] `bool to(T value)`
- [ ] `bool up()`
- [ ] `bool down()`
- [ ] `bool ltoh()`
- [ ] `bool htol()`
- [ ] `bool held(uint16_t ms, T value, bool release = false)`
- [ ] `uint32_t elapsed() const`

**privateメソッド**

- `template <typename U> static bool nearValue(U a, U b, U tolerance)`
- `static bool nearValue(float a, float b, float tolerance)`
- `static bool nearValue(double a, double b, double tolerance)`
- `bool same(T a, T b) const`
- `void initialize(T value, uint32_t now)`
- `void commit(T value, uint32_t now)`

### class `Di`（public InEdge）

- [ ] デジタル入力

**publicメソッド**

- [ ] `explicit Di(uint8_t pin, uint16_t lock = 10)`
- [ ] `~Di()`
- [ ] `Di(const Di&) = delete` — 削除定義（呼び出し不可）
- [ ] `Di& operator=(const Di&) = delete` — 削除定義（呼び出し不可）

**privateメソッド**

- `static void serviceAll(uint32_t now)`
- `static void serviceEvents(uint32_t epoch)`

### class `Pr`（public InEdge）

- [ ] しきい値付きアナログ入力／フォトリフレクタ

**publicメソッド**

- [ ] `explicit Pr(uint8_t pin, int threshold = 950, uint16_t lock = 10)`
- [ ] `~Pr()`
- [ ] `Pr(const Pr&) = delete` — 削除定義（呼び出し不可）
- [ ] `Pr& operator=(const Pr&) = delete` — 削除定義（呼び出し不可）
- [ ] `int raw() const`

**privateメソッド**

- `static void serviceAll(uint32_t now)`
- `static void serviceEvents(uint32_t epoch)`

### class `Sok`

- [ ] 距離センサ入力

**publicメソッド**

- [ ] `explicit Sok(uint8_t pin, uint16_t lock = 10)`
- [ ] `~Sok()`
- [ ] `Sok(const Sok&) = delete` — 削除定義（呼び出し不可）
- [ ] `Sok& operator=(const Sok&) = delete` — 削除定義（呼び出し不可）
- [ ] `int raw() const`
- [ ] `int mm() const`
- [ ] `float cm() const`

**privateメソッド**

- `void serviceOne(uint32_t now)`
- `static void serviceAll(uint32_t now)`

### class `Vr`

- [ ] 半固定抵抗器入力と範囲変換

**publicメソッド**

- [ ] `explicit Vr( uint8_t pin, int minValue = 0, int maxValue = 512, uint16_t lock = 10 )`
- [ ] `~Vr()`
- [ ] `Vr(const Vr&) = delete` — 削除定義（呼び出し不可）
- [ ] `Vr& operator=(const Vr&) = delete` — 削除定義（呼び出し不可）
- [ ] `int raw() const`
- [ ] `int to(int lo, int hi) const`

**privateメソッド**

- `void serviceOne(uint32_t now)`
- `static void serviceAll(uint32_t now)`

### class `Js`

- [ ] 2軸ジョイスティック入力と方向判定

**publicメソッド**

- [ ] `Js(uint8_t px, uint8_t py, uint16_t lock = 10)`
- [ ] `~Js()`
- [ ] `Js(const Js&) = delete` — 削除定義（呼び出し不可）
- [ ] `Js& operator=(const Js&) = delete` — 削除定義（呼び出し不可）
- [ ] `int x() const`
- [ ] `int y() const`
- [ ] `void read(int& vx, int& vy) const`
- [ ] `int dir(int div, uint8_t rot = 0, bool mirror = false) const`

**privateメソッド**

- `void serviceOne(uint32_t now)`
- `static void serviceAll(uint32_t now)`

### class `Enc`

- [ ] ロータリーエンコーダ入力

**publicメソッド**

- [ ] `Enc(uint8_t pa, uint8_t pb, bool d = true, uint16_t lock = 10)`
- [ ] `~Enc()`
- [ ] `Enc(const Enc&) = delete` — 削除定義（呼び出し不可）
- [ ] `Enc& operator=(const Enc&) = delete` — 削除定義（呼び出し不可）
- [ ] `int32_t delta()`
- [ ] `int32_t clampTo( int32_t value, int32_t lo, int32_t hi )`
- [ ] `int32_t loopTo( int32_t value, int32_t lo, int32_t hi )`

**privateメソッド**

- `inline bool acceptEvent()`
- `inline void poll()`
- `int32_t take()`
- `static void acquireTimer()`
- `static void releaseTimer()`
- `static void beginPolling(bool forceTimer = false)`
- `static inline void isrPollFallback()`
- `static inline void isrPcint(uint8_t group)`

### class `Led`

- [ ] RGB LED出力

**publicメソッド**

- [ ] `Led()`
- [ ] `Led& operator()(uint8_t newColor = 0)`
- [ ] `Led& operator()(bool g, bool b, bool r)`
- [ ] `Led& per(int opacityPercent)`

**privateメソッド**

- `void prepareDraft()`
- `void writeState(uint8_t state)`
- `Led& set(uint8_t newColor, uint8_t newOpacity)`
- `void commit()`
- `void serviceTick()`

### class `Disp`

- [ ] 3桁7セグメントLED出力

**publicメソッド**

- [ ] `Disp()`
- [ ] `Disp& operator()(uint8_t a, uint8_t b, uint8_t c)`
- [ ] `Disp& off()`
- [ ] `Disp& s(const char* text)`
- [ ] `Disp& n(int x, bool zero = false, bool left = false)`
- [ ] `Disp& f(double f, bool zero = false, bool left = false)`
- [ ] `Disp& per(int oa, int ob, int oc)`
- [ ] `Disp& per(int oa, int ob)`
- [ ] `Disp& per(int oa)`
- [ ] `Disp& base(int32_t x, uint8_t radix, bool zero = false)`
- [ ] `template <size_t NA, size_t NB, size_t NC> Disp& art( const char(&a)[NA], const char(&b)[NB], const char(&c)[NC] )`
- [ ] `Disp& art(const char* const p[3])`

**privateメソッド**

- `void prepareDraft()`
- `void commit()`
- `static uint8_t toPattern(char c)`
- `static uint8_t digitPattern(uint8_t value)`
- `static uint8_t artPattern(const char* s)`
- `void serviceTick()`

### class `Dcm`

- [ ] DCモータ出力

**publicメソッド**

- [ ] `Dcm()`
- [ ] `int8_t dir() const`
- [ ] `void cw(int spd)`
- [ ] `void cw(int spd, uint32_t durationMs)`
- [ ] `void ccw(int spd)`
- [ ] `void ccw(int spd, uint32_t durationMs)`
- [ ] `void br()`
- [ ] `void fr()`
- [ ] `bool busy() const`
- [ ] `bool done()`

**privateメソッド**

- `bool repeatsTimedCommand( int8_t direction, uint8_t pwm, uint32_t durationMs )`
- `void rememberTimedCommand( int8_t direction, uint8_t pwm, uint32_t durationMs )`
- `inline void stopFromIsr()`
- `inline void isrTick()`

### class `Spm`

- [ ] ステッピングモータ出力

**publicメソッド**

- [ ] `explicit Spm(Excitation mode = TWO_PHASE)`
- [ ] `int8_t dir() const`
- [ ] `void cw()`
- [ ] `void ccw()`
- [ ] `void fr()`
- [ ] `void br()`
- [ ] `void _one()`
- [ ] `void _two()`
- [ ] `void rela(float degree)`
- [ ] `void abso(float degree, Dir dir = SHORT, Dir halfDir = CCW)`
- [ ] `void update(uint32_t intervalMs)`
- [ ] `bool busy() const`
- [ ] `void stop()`
- [ ] `void zero()`
- [ ] `float pos() const`
- [ ] `int32_t stepPos() const`

**privateメソッド**

- `void phase(uint8_t s)`
- `void mode(Excitation newMode)`
- `int32_t degreeToStep(float degree) const`
- `void stepCw()`
- `void stepCcw()`

### class `Bz`

- [ ] ブザー出力とメロディ再生

**publicメソッド**

- [ ] `Bz()`
- [ ] `void operator()(int frequency)`
- [ ] `void operator()(int f, uint32_t durationMs)`
- [ ] `void play(const int* notes, const int* durations, int length, bool repeat = false)`
- [ ] `void stop()`
- [ ] `void off()`
- [ ] `bool playing() const`

**privateメソッド**

- `bool repeatsTimedCommand(int frequency, uint32_t durationMs)`
- `static uint16_t topForFrequency(int f)`
- `void start(int f, uint32_t durationMs, bool timed)`
- `inline void stopFromIsr()`
- `void update()`
- `inline void isrTick()`

### class `Seq`

- [ ] loop内の状態列を管理するシーケンサ

**publicメソッド**

- [ ] `Seq() = default`
- [ ] `Seq(const Seq&) = delete` — 削除定義（呼び出し不可）
- [ ] `Seq& operator=(const Seq&) = delete` — 削除定義（呼び出し不可）
- [ ] `bool on()`
- [ ] `bool operator()()`
- [ ] `explicit operator bool()`
- [ ] `void next()`
- [ ] `void prev()`
- [ ] `void toa(int state)`
- [ ] `void tor(int offset)`
- [ ] `void restart()`
- [ ] `bool is(int state)`
- [ ] `int now()`
- [ ] `int steps()`
- [ ] `bool in()`
- [ ] `bool out()`
- [ ] `uint32_t elapsed()`
- [ ] `bool after(uint32_t ms)`

**privateメソッド**

- `int clampState(int state) const`
- `void syncLoop()`
- `void moveTo(int state)`

### class `Iv`

- [ ] 繰り返し間隔タイマー

**publicメソッド**

- [ ] `bool operator()(uint32_t ms)`
- [ ] `void reset(bool immediate = true)`
- [ ] `void wait()`
- [ ] `void go()`
- [ ] `bool isWait() const`

### class `Ti`

- [ ] ワンショットタイマー

**publicメソッド**

- [ ] `void start(uint32_t ms)`
- [ ] `void stop()`
- [ ] `bool active() const`
- [ ] `bool done()`
- [ ] `uint32_t remain() const`

### class `Sw`

- [ ] 累積型ストップウォッチ

**publicメソッド**

- [ ] `void start()`
- [ ] `void stop()`
- [ ] `void reset()`
- [ ] `bool running() const`
- [ ] `uint32_t ms() const`
- [ ] `double s() const`
- [ ] `uint32_t operator()() const`
- [ ] `operator uint32_t() const`

### class `Tog`

- [ ] 呼び出すたびに状態を反転するトグル

**publicメソッド**

- [ ] `explicit Tog(bool initial = true)`
- [ ] `bool operator()()`
- [ ] `template <class T> T operator()(T first, T second)`
- [ ] `operator bool() const`
- [ ] `bool get() const`
- [ ] `void flip()`
- [ ] `void reset(bool initial = true)`

### struct `board_detail::AdcSlot`

- ADC走査1枠を表す内部構造体

## 3. 関数形式マクロ・型別名・呼び出し可能な共有オブジェクト

- [ ] `using Sig = SigValue<int32_t>` — 標準の信号型別名
- [ ] `sig(value, ...)` — 呼び出し位置ごとの静的`SigValue`を生成・更新するマクロ
- [ ] `tog(...)` — 呼び出し位置ごとの静的`Tog`を呼び出すマクロ
- [ ] `setup` → `userSetup` — ユーザー定義関数名の置換
- [ ] `loop` → `userLoop` — ユーザー定義関数名の置換
- [ ] `D(...)` — デバッグ用の複数値出力
- [ ] `DP(x)` — 改行なしデバッグ出力
- [ ] `DV(x)` — 変数名と値のデバッグ出力
- [ ] `DT(x)` — 時刻付きデバッグ出力
- [ ] `DC(x)` — 値が変化したときだけ出力
- [ ] `DH()` — ファイル名と行番号のデバッグ出力
- [ ] `led` — `Led`共有オブジェクト
- [ ] `dp` — `Disp`共有オブジェクト
- [ ] `dm` — `Dcm`共有オブジェクト
- [ ] `sm` — `Spm`共有オブジェクト
- [ ] `bz` — `Bz`共有オブジェクト
