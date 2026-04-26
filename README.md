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
<summary><h4><s>arduino-cliのセットアップ手順</s></h4></summary>

- .arduino-cliディレクトリをコピーし、環境変数に登録する。
- ```arduino-cli config init```
- ```arduino-cli core update-index```
- ```arduino-cli core install arduino:avr```
- ```arduino-cli board list```

</details>

<details>
<summary><h4>vim</h4></summary>

- `"yp` レジスタpに選択位置をヤンク
- `"pp` レジスタpの内容を貼り付け

</details>

[mono_con.h](./mono_con.h)