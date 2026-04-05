<!-- markdownlint-disable -->

<details>
<summary><h4>割り込みの書き方</summary>

```c++
#define USE_TIMER3_ISR
#include "mono_con.h"

word tc;

ISR (TIMER3_COMPA_vect) {
	tc++;
}

void loop() {
	if (tc > 1000) {
		tc = 0;
		// 1秒毎に実行
	}
}
```

</details>

<details>
<summary><h4>arduino-cliのセットアップ手順</summary>

- .arduino-cliディレクトリをコピーし、環境変数に登録する。
- ```bash arduino-cli config init``` (PowerShell)
- ```bash arduino-cli core update-index```
- ```bash arduino-cli core install arduino:avr```
- ```bash arduino-cli board list```

</details>